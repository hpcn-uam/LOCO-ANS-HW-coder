/*
  Copyright 2021 Tobías Alonso, Autonomous University of Madrid
  This file is part of LOCO-ANS HW encoder.
  LOCO-ANS HW encoder is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  LOCO-ANS HW encoder is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with LOCO-ANS HW encoder.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "LOCO_decorrelator.hpp"
#include "LOCO_functions.hpp"
#include "context.hpp"


template<int IS>
unsigned int hw_floor_log2(ap_uint<IS> in){
  #pragma HLS inline
  unsigned int out =0;
  for(unsigned i = 1; i < IS; ++i) {
    #pragma HLS UNROLL
    if(in[i]==1) {
      out = i;
    }
  }
  return  out;
}

void LOCO_decorrelator(
  col_ptr_t config_cols,
  row_ptr_t config_rows,
  ap_uint<8> config_near,
  hls::stream<px_t>& src,
  hls::stream<px_t>& first_px_out,
  hls::stream<DecorrelatorOutput> & symbols,
  //READ-ONLY (from the user perspective) AXI-LITE registers
  ap_uint<8> &param_max_near,
  ap_uint<8> &param_num_of_tiles){
  param_max_near = MAX_NEAR;
  param_num_of_tiles=1;

  #ifdef LOCO_DECORRELATOR_TOP
  //interface configuration
  #pragma HLS INTERFACE axis register_mode=both register port=src
  #pragma HLS INTERFACE axis register_mode=both register port=symbols
  #pragma HLS INTERFACE axis register_mode=both register port=first_px_out
  #pragma HLS INTERFACE s_axilite port=config_near bundle=config
  #pragma HLS INTERFACE s_axilite port=config_rows bundle=config
  #pragma HLS INTERFACE s_axilite port=config_cols bundle=config

  //READ-ONLY (from the user perspective) AXI-LITE registers
  #pragma HLS INTERFACE s_axilite port=param_max_near bundle=config
  #pragma HLS INTERFACE ap_none port=param_max_near
  #pragma HLS INTERFACE s_axilite port=param_num_of_tiles bundle=config
  #pragma HLS INTERFACE ap_none port=param_num_of_tiles


  #pragma HLS INTERFACE s_axilite port=return bundle=config
  // #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif

  image_buffer<BUFFER_COLS> quantized_img;
  #pragma HLS BIND_STORAGE variable=quantized_img.buffer type=ram_2p impl=bram latency=1

  // copy args
  auto cols = config_cols;
  auto rows = config_rows;
  int near  = config_near;
  int img_pixels = cols*rows;

  quantized_img.init(cols);

  //context init
  // constexpr int near =0,
  const int delta =(near<<1)+1;
  // OPT: limit near and replace alpha computation with a lut
  const int alpha = near ==0?MAXVAL + 1 :
               (MAXVAL + 2 * near) / delta + 1;

  const int MIN_REDUCT_ERROR = -near;
  const int MAX_REDUCT_ERROR =  MAXVAL + near;
  const int DECO_RANGE = alpha * delta;
  const int MAX_ERROR =  (alpha+1)/2 -1; //  std::ceil(alpha/2.0) -1;
  const int MIN_ERROR = -(alpha/2); // -std::floor(alpha/2.0);
  const unsigned int remainder_reduct_bits = hw_floor_log2<10>(delta); //near<=255 -> 10 bits for delta
  init_context(near,alpha);

  ContextElement prev_ctx_stats;
  int prev_context=511;//not a context;
  // #pragma HLS disaggregate variable=context_stats
  #pragma HLS aggregate variable=context_stats
  #pragma HLS disaggregate variable=prev_ctx_stats

  //first pixel process
    px_t first_px = src.read();
    first_px_out << first_px;
    // quantized_img.update(first_px);

  int q_channel_value = first_px;

  px_loop: for (int px_idx = 1; px_idx < img_pixels; ++px_idx){
    #pragma HLS PIPELINE II=2
    #pragma HLS LOOP_TRIPCOUNT max=9999 //just a number to quickly be able to estimate efficiency

    #pragma HLS DEPENDENCE variable=quantized_img.buffer intra false
    #pragma HLS DEPENDENCE variable=quantized_img.buffer inter false

      auto channel_value = src.read();
      quantized_img.update(q_channel_value);

      int fixed_prediction;
      int ctx_id;
      int ctx_sign;
      quantized_img.get_fixed_prediction_and_context(ctx_id,fixed_prediction,ctx_sign);

      //correct prediction
      ContextElement ctx_stats=  context_stats[ctx_id] ;
      // ContextElement ctx_stats= prev_context == ctx_id?prev_ctx_stats: context_stats[ctx_id] ;
      // prev_context = ctx_id;
      #pragma HLS disaggregate variable=ctx_stats

      int prediction_correction = ctx_sign == 0? ap_int<ctx_bias_t::width+1>(ctx_stats.bias ):
                                                -ctx_stats.bias;
      int prediction = clamp(prediction_correction + fixed_prediction);

      #if CTX_ADJUST_CNT_IN_NEXT_ITER
        if(ctx_stats.cnt == 0 ) {
          ctx_stats.cnt = CTX_ADJUST_CNT>>1;
          ctx_stats.Nt  >>=1;
          ctx_stats.acc = (ctx_stats.acc >= 0)? ctx_acc_t(ctx_stats.acc >> 1): ctx_acc_t((1 + ctx_stats.acc) >> 1);
          ctx_stats.St  >>=1;
        }
      #endif

      int error = channel_value - prediction;
      int acc_inv_sign = (ctx_stats.acc > 0)? 1:0;

      #if FORWARD_QUANTIZATION_ERROR
        auto quantization_error = quant_error_lut[ap_uint<INPUT_BPP+1>(error)];
      #endif
      error = acc_inv_sign^ctx_sign ? -error: error;

      #ifdef DEBUG
        int orig_error = error;
        int dgb_error = error;
        dgb_error = Uniform_quantizer(dgb_error,  delta, near);
        if((dgb_error < MIN_ERROR)){
          dgb_error += alpha;
        }else if((dgb_error > MAX_ERROR)){
          dgb_error -= alpha;
        }
      #endif

      #if USING_DIV_RED_LUT
        ap_uint<INPUT_BPP+1> lut_address = error;
        error = quant_reduct_lut[lut_address].reduct_error ;
        ap_int<INPUT_BPP+1> q_error = quant_reduct_lut[lut_address].reconstruct_error;//error*delta;
        #pragma HLS disaggregate variable=quant_reduct_lut
      #else
        // error = UQ(error, delta,near ); // quantize

        if((error < MIN_ERROR)){
          error += alpha;
        }else if((error > MAX_ERROR)){
          error -= alpha;
        }

        int q_error = error*delta;
      #endif

      ASSERT(MIN_ERROR,<=,error)
      ASSERT(error,<=,MAX_ERROR)

      #ifdef DEBUG
        ASSERT(dgb_error,==,error,"px_idx:"<<px_idx<<"| orig_error: "<<orig_error);
        ASSERT(dgb_error*delta,==,q_error,"px_idx:"<<px_idx<<"| orig_error: "<<orig_error);
      #endif

      int y = error <0? 1:0;
      ap_uint<Z_SIZE> z = abs(error)-y;


      // q_error = (acc_inv_sign ? - q_error: q_error);
      q_error = (acc_inv_sign ? ap_int<INPUT_BPP+1>(- q_error): q_error);

      #if FORWARD_QUANTIZATION_ERROR
        q_channel_value = channel_value + quantization_error;
      #else
        q_channel_value = prediction + (ctx_sign==1? ap_int<INPUT_BPP+1>(-q_error):q_error);

        #if ERROR_REDUCTION
          if((q_channel_value < MIN_REDUCT_ERROR)){
            q_channel_value += DECO_RANGE;
          }else if((q_channel_value > MAX_REDUCT_ERROR)){
            q_channel_value -= DECO_RANGE;
          }
        #endif
      #endif

      q_channel_value = clamp(q_channel_value);

      ASSERT(abs(q_channel_value-channel_value),<=,near,"px_idx: "<<px_idx<<
          " | q_channel_value: "<<int(q_channel_value)<<" | channel_value: "<<int(channel_value)
          <<" | q_error: "<<q_error<<" | error: "<<error);

      int last_symbol = px_idx == img_pixels-1? 1:0;

      DecorrelatorOutput out_symbol(remainder_reduct_bits,ctx_stats.St,ctx_stats.cnt,
              ctx_stats.p_idx, y,z,last_symbol);
      symbols << out_symbol;

      //update encoder variables
        update_context(ctx_id,q_error,y,z,ctx_stats,prev_ctx_stats);


  }
}


void LOCO_decorrelator_LS(
  col_ptr_t config_cols,
  row_ptr_t config_rows,
  hls::stream<px_t>& src,
  hls::stream<px_t>& first_px_out,
  hls::stream<DecorrelatorOutput> & symbols){

  #ifdef LOCO_DECORRELATOR_LS_TOP
  //interface configuration
  #pragma HLS INTERFACE axis register_mode=both register port=src
  #pragma HLS INTERFACE axis register_mode=both register port=symbols
  #pragma HLS INTERFACE axis register_mode=both register port=first_px_out
  #pragma HLS INTERFACE s_axilite port=config_rows bundle=config
  #pragma HLS INTERFACE s_axilite port=config_cols bundle=config
  #pragma HLS INTERFACE s_axilite port=return bundle=config
  // #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif

  image_buffer<BUFFER_COLS> quantized_img;
  #pragma HLS BIND_STORAGE variable=quantized_img.buffer type=ram_2p impl=bram latency=1

  // copy args
  auto cols = config_cols;
  auto rows = config_rows;
  int img_pixels = cols*rows;

  quantized_img.init(cols);

  //context init
  constexpr int near =0, delta =1;
  constexpr int alpha = near ==0?MAXVAL + 1 :
                     (MAXVAL + 2 * near) / delta + 1;

  constexpr int MIN_REDUCT_ERROR = -near;
  constexpr int MAX_REDUCT_ERROR =  MAXVAL + near;
  constexpr int DECO_RANGE = alpha * delta;
  constexpr int MAX_ERROR =  (alpha+1)/2 -1; //  std::ceil(alpha/2.0) -1;
  constexpr int MIN_ERROR = -(alpha/2); // -std::floor(alpha/2.0);
  constexpr unsigned int remainder_reduct_bits = 0;
  init_context(near,alpha);

  ContextElement prev_ctx_stats;
  int prev_context=511;//not a context;
  // #pragma HLS disaggregate variable=context_stats
  #pragma HLS aggregate variable=context_stats
  #pragma HLS disaggregate variable=prev_ctx_stats

  //first pixel process
    px_t first_px = src.read();
    first_px_out << first_px;

  int q_channel_value = first_px;

  px_loop: for (int px_idx = 1; px_idx < img_pixels; ++px_idx){
    #pragma HLS PIPELINE II=1
    // #pragma HLS PIPELINE II=2
    #pragma HLS LOOP_TRIPCOUNT max=9999 //just a number to quickly be able to estimate efficiency

    #pragma HLS DEPENDENCE variable=quantized_img.buffer intra false
    #pragma HLS DEPENDENCE variable=quantized_img.buffer inter false
    // #pragma HLS DEPENDENCE variable=quantized_img.b inter false
    // #pragma HLS DEPENDENCE variable=quantized_img.b_reg inter false
    // #pragma HLS DEPENDENCE variable=quantized_img.b_reg intra false
    // #pragma HLS DEPENDENCE variable=context_stats distance=2 direction=RAW type=inter true
    // #pragma HLS DEPENDENCE variable=context_stats distance=1 direction=RAW type=inter false

      auto channel_value = src.read();
      quantized_img.update(q_channel_value);
      q_channel_value= channel_value;

      int fixed_prediction;
      int ctx_id;
      int ctx_sign;
      quantized_img.get_fixed_prediction_and_context(ctx_id,fixed_prediction,ctx_sign);

      //correct prediction
      ContextElement ctx_stats=  context_stats[ctx_id] ;
      #if CTX_ADJUST_CNT_IN_NEXT_ITER
        if(ctx_stats.cnt == 0 ) {
        // if(ctx_stats.cnt == CTX_ADJUST_CNT ) {
          // ctx_stats.cnt >>=1;
          ctx_stats.cnt = CTX_ADJUST_CNT>>1;
          ctx_stats.Nt  >>=1;
          ctx_stats.acc = (ctx_stats.acc >= 0)? ctx_acc_t(ctx_stats.acc >> 1): ctx_acc_t((1 + ctx_stats.acc) >> 1);
          ctx_stats.St  >>=1;
        }
      #endif
      // ContextElement ctx_stats= prev_context == ctx_id?prev_ctx_stats: context_stats[ctx_id] ;
      prev_context = ctx_id;
      #pragma HLS disaggregate variable=ctx_stats


      int prediction_correction = ctx_sign == 0? ap_int<ctx_bias_t::width+1>(ctx_stats.bias ):
                                                -ctx_stats.bias;
      int prediction = clamp(prediction_correction + fixed_prediction);
      int error = channel_value - prediction;
      int acc_inv_sign = (ctx_stats.acc > 0)? 1:0;
      error = acc_inv_sign^ctx_sign ? -error: error;

      if((error < MIN_ERROR)){
        error += alpha;
      }else if((error > MAX_ERROR)){
        error -= alpha;
      }

      ASSERT(error<= MAX_ERROR );
      ASSERT(error>= MIN_ERROR  );

      // int red_error = error;
      ap_int<INPUT_BPP> red_error = error;
      int q_error = red_error;


      int y = red_error <0? 1:0;
      // int z = abs(red_error)-y;
      ap_uint<Z_SIZE> z = abs(red_error)-y;


      q_error = (acc_inv_sign ? - q_error: q_error);

      ASSERT(abs(q_channel_value-channel_value),<=,near,"px_idx: "<<px_idx<<
          " | q_channel_value: "<<int(q_channel_value)<<" | channel_value: "<<int(channel_value)
          <<" | q_error: "<<q_error<<" | error: "<<error);

      int last_symbol = px_idx == img_pixels-1? 1:0;
      DecorrelatorOutput out_symbol(remainder_reduct_bits,ctx_stats.St,ctx_stats.cnt,
          ctx_stats.p_idx, y,z,last_symbol);
      symbols << out_symbol;

      //update encoder variables
        update_context(ctx_id,q_error,y,z,ctx_stats,prev_ctx_stats);


  }
}




void St_idx_compute(
  hls::stream<DecorrelatorOutput> & in_symbols,
  hls::stream<coder_interf_t> & out_symbols){
  #ifdef ST_IDX_COMPUTE_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=in_symbols
  #pragma HLS INTERFACE axis register_mode=both register port=out_symbols
  #endif

  #pragma HLS INTERFACE ap_ctrl_none port=return
  // #pragma HLS PIPELINE
  #pragma HLS PIPELINE style=flp
  // START_SW_ONLY_LOOP(!in_symbols.empty())



  ap_uint<THETA_SIZE> theta_id;
  DecorrelatorOutput in_symbol = in_symbols.read();

  ap_uint<REM_REDUCT_SIZE> red_bits = in_symbol.remainder_reduct_bits();
  ap_uint<Z_SIZE> z = in_symbol.z();
  ap_uint<Y_SIZE> y = in_symbol.y();
  ap_uint<P_SIZE> p_id = in_symbol.p_idx();
  ap_uint<1> last = in_symbol.last();

  auto cnt = in_symbol.cnt();
  auto St = in_symbol.St();

  #if CTX_ST_FINER_QUANT
    int idx = 0;
    // for(e = 0; St > l; l<<=1,e+=2) {;}
    for(unsigned i = 2; i <= MAX_ST_IDX; i+=2) {
      if(St > cnt) {
        idx = i;
        cnt<<=1;
      }
    }

    if(St> cnt-((cnt+2)>>2)){
      #if MAX_ST_IDX % 2 == 1
      idx |=1;
      #else
      if(idx< MAX_ST_IDX) {
        idx |=1;
      }
      #endif
    }

    theta_id = idx;
  #else
    // for(theta_id = 0; St > (cnt<<(theta_id)); ++theta_id) {;}
    #if 1
    theta_id = 0;
    for(unsigned i = 1; i <= MAX_ST_IDX; ++i) {
      if(St > (cnt<<(i-1))) {
        theta_id = i;
      }
    }
    #else
    // This is a bit faster but haven't tested in different configurations
    // OPT: HLS doesn't seem to be building a balanced tree, so a tree
    // implementation using binary search might be the optimum way of doing this
    constexpr int MIDDLE = (MAX_ST_IDX/2);
    if(St > (cnt<<(MIDDLE-1))) {
      theta_id = MIDDLE;
      for(unsigned i = MIDDLE+1; i <= MAX_ST_IDX; ++i) {
        if(St > (cnt<<(i-1))) {
          theta_id = i;
        }
      }
    }else{
      theta_id = 0;
      for(unsigned i = 1; i <= MIDDLE-1; ++i) {
        if(St > (cnt<<(i-1))) {
          theta_id = i;
        }
      }
    }
    #endif
  #endif

  ASSERT(theta_id, <=,MAX_ST_IDX)
  out_symbols << (last,red_bits,z,y,theta_id,p_id);

  // END_SW_ONLY_LOOP

}

/*void LOCO_decorrelator(
  col_ptr_t config_cols,
  row_ptr_t config_rows,
  hls::stream<px_t>& src,
  hls::stream<px_t>& first_px_out,
  hls::stream<coder_interf_t> & out_symbols){

  //interface configuration
  #pragma HLS INTERFACE axis register_mode=both register port=src
  #pragma HLS INTERFACE axis register_mode=both register port=first_px_out
  #pragma HLS INTERFACE axis register_mode=both register port=out_symbols
  #pragma HLS INTERFACE s_axilite port=config_cols bundle=config
  #pragma HLS INTERFACE s_axilite port=config_rows bundle=config
  #pragma HLS INTERFACE s_axilite port=return bundle=config

  #pragma HLS DATAFLOW disable_start_propagation

  hls::stream<DecorrelatorOutput>  pre_symbols;
  LOCO_decorrelator_1(config_cols,config_rows,src,first_px_out,pre_symbols);
  St_idx_compute(pre_symbols,out_symbols);
}*/

/*void LOCO_decorrelator(
  col_ptr_t config_cols,
  row_ptr_t config_rows,
  ap_uint<32> num_of_images,
  hls::stream<px_t>& src,
  hls::stream<err_t> & symbols){
  #pragma HLS INTERFACE axis register_mode=both register port=src
  #pragma HLS INTERFACE axis register_mode=both register port=symbols
  #pragma HLS INTERFACE s_axilite port=config_rows bundle=config
  #pragma HLS INTERFACE s_axilite port=num_of_images bundle=config
  #pragma HLS INTERFACE s_axilite port=config_cols bundle=config

  auto cols = config_cols;
  auto rows = config_rows;
  auto images = config_images;
  unsigned int img_pixels = cols*rows;

  for(unsigned i = 0; i < images; ++i) {

    #pragma HLS LOOP_TRIPCOUNT max=100 //just a number to quickly be able to estimate efficiency
    // #pragma HLS DATAFLOW
    LOCO_decorrelator_stage_1(cols,img_pixels,src,symbols)
  }

}*/

/*void LOCO_decorrelator(
  col_ptr_t config_cols,
  row_ptr_t config_rows,
  hls::stream<px_t>& src,
  hls::stream<err_t> & symbols){
    #pragma HLS INTERFACE axis register_mode=both register port=src
  #pragma HLS INTERFACE axis register_mode=both register port=symbols
  #pragma HLS INTERFACE s_axilite port=config_cols bundle=config
  #pragma HLS INTERFACE s_axilite port=config_rows bundle=config
  // #pragma HLS INTERFACE ap_ctrl_none port=return

  #pragma HLS DATAFLOW

  ctx_bias_t context_bias[NUM_OF_CTX];
  #pragma HLS STREAM variable=context_bias type=pipo depth=2
  // #pragma HLS STREAM variable=context_bias type=unsync depth=2

  init_context_parallel(context_bias);
  LOCO_decorrelator_fn(config_cols,config_rows,context_bias, src, symbols);
}*/
