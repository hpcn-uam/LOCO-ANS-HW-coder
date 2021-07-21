
#include "LOCO_decorrelator.hpp"
#include "LOCO_functions.hpp"
#include "context.hpp"


#if END_OF_LINE_CALL

// void LOCO_decorrelator_fn(
void LOCO_decorrelator(
  col_ptr_t config_cols,
  row_ptr_t config_rows,
  // ctx_bias_t context_bias[NUM_OF_CTX],
  hls::stream<px_t>& src,
  hls::stream<err_t> & symbols){
// #pragma HLS INTERFACE mode=bram latency=1 port=context_bias storage_type=ram_1p


  //interface configuration
  #pragma HLS INTERFACE axis register_mode=both register port=src
  #pragma HLS INTERFACE axis register_mode=both register port=symbols
  #pragma HLS INTERFACE s_axilite port=config_cols bundle=config
  #pragma HLS INTERFACE s_axilite port=config_rows bundle=config
  // #pragma HLS INTERFACE ap_ctrl_none port=return

  image_buffer<BUFFER_COLS> quantized_img;
  #pragma HLS BIND_STORAGE variable=quantized_img.buffer type=ram_2p impl=bram latency=1

  // copy args
  auto cols = config_cols;
  auto rows = config_rows;


  //context init
  init_context();

  ContextElement prev_ctx_stats;
  int prev_context=511;//not a context;
  #pragma HLS disaggregate variable=context_stats
  #pragma HLS disaggregate variable=prev_ctx_stats
  
  
  quantized_img.init(rows,cols);
  bool first_px = true;
  row_loop:for(unsigned row = 0; row < rows; ++row) {
    #pragma HLS LOOP_FLATTEN off
    #pragma HLS LOOP_TRIPCOUNT max=100 //just a number to quickly be able to estimate efficiency 
    col_loop:for(unsigned col = 0; col < cols; ++col) {
      // #pragma HLS DEPENDENCE variable=quantized_img.buffer intra false.
      #pragma HLS LOOP_TRIPCOUNT max=100 //just a number to quickly be able to estimate efficiency 
      // #pragma HLS PIPELINE rewind
      #pragma HLS PIPELINE 
      // #pragma HLS DEPENDENCE variable=context_stats distance=1 direction=RAW type=inter false  

      auto channel_value = src.read(); 
      int q_channel_value;
      int q_idx;
      if (first_px){ // OPT use first_px flag
        first_px = false;
        q_channel_value= channel_value;
        q_idx=channel_value;
      }else{

        int fixed_prediction;
        int ctx_id;
        int ctx_sign;
        quantized_img.get_fixed_prediction(ctx_id,fixed_prediction,ctx_sign);

        //correct prediction
        // ContextElement ctx_stats=  context_stats[ctx_id] ;
        ContextElement ctx_stats= prev_context == ctx_id?prev_ctx_stats: context_stats[ctx_id] ;
        #pragma HLS disaggregate variable=ctx_stats
        
        #if 1
          int prediction_correction = ctx_sign == 0? ap_int<ctx_bias_t::width+1>(ctx_stats.bias ): 
                                                    -ctx_stats.bias;
          int prediction = clamp(prediction_correction + fixed_prediction);
        #else
          int prediction = fixed_prediction;
        #endif

        int error = channel_value - prediction;
        
        int acc_inv_sign = (ctx_stats.acc > 0)? 1:0;
        error = acc_inv_sign^ctx_sign ? -error: error;
        int q_error ;

        #if 0
          UQ_n_reduce(error,delta,q_idx,q_error);
        #else
          q_error = acc_inv_sign ? - error: error;
          q_idx = error;
        #endif

        q_channel_value = clamp(prediction + (ctx_sign==1? -q_error:q_error));

       
        //update encoder variables 
          update_context(ctx_id,q_error,ctx_stats,prev_ctx_stats);
      }

      quantized_img.update(q_channel_value);
      symbols << q_idx;


    }
    quantized_img.end_of_line();
  }

}

#else

void LOCO_decorrelator(
  col_ptr_t config_cols,
  row_ptr_t config_rows,
  hls::stream<px_t>& src,
  hls::stream<err_t> & symbols){ 

  //interface configuration
  #pragma HLS INTERFACE axis register_mode=both register port=src
  #pragma HLS INTERFACE axis register_mode=both register port=symbols
  #pragma HLS INTERFACE s_axilite port=config_rows bundle=config
  #pragma HLS INTERFACE s_axilite port=config_cols bundle=config
  // #pragma HLS INTERFACE ap_ctrl_none port=return


  image_buffer<BUFFER_COLS> quantized_img;
  #pragma HLS BIND_STORAGE variable=quantized_img.buffer type=ram_2p impl=bram latency=1

  // copy args
  auto cols = config_cols;
  auto rows = config_rows;
  quantized_img.init(rows,cols);


  //context init
  init_context();

  ContextElement prev_ctx_stats;
  int prev_context=511;//not a context;
  #pragma HLS disaggregate variable=context_stats
  #pragma HLS disaggregate variable=prev_ctx_stats

  //first pixel process 
    px_t first_px = src.read();
    symbols << err_t(first_px);
    // quantized_img.update(first_px);

    int q_channel_value = first_px;
  int img_pixels = cols*rows;
  px_loop: for (int px_idx = 1; px_idx < img_pixels; ++px_idx){
    #pragma HLS PIPELINE
    #pragma HLS LOOP_TRIPCOUNT max=9999 //just a number to quickly be able to estimate efficiency 

    #pragma HLS DEPENDENCE variable=quantized_img.buffer intra false
    // #pragma HLS DEPENDENCE variable=quantized_img.b inter false
    #pragma HLS DEPENDENCE variable=quantized_img.b_reg inter false
    #pragma HLS DEPENDENCE variable=quantized_img.b_reg intra false
    #pragma HLS DEPENDENCE variable=context_stats distance=1 direction=RAW type=inter false  

      auto channel_value = src.read(); 
      quantized_img.update(q_channel_value);
      int q_idx;


      int fixed_prediction;
      int ctx_id;
      int ctx_sign;
      quantized_img.get_fixed_prediction(ctx_id,fixed_prediction,ctx_sign);

      //correct prediction
      // ContextElement ctx_stats=  context_stats[ctx_id] ;
      ContextElement ctx_stats= prev_context == ctx_id?prev_ctx_stats: context_stats[ctx_id] ;
      #pragma HLS disaggregate variable=ctx_stats
      
      #if 1
        int prediction_correction = ctx_sign == 0? ap_int<ctx_bias_t::width+1>(ctx_stats.bias ): 
                                                  -ctx_stats.bias;
        int prediction = clamp(prediction_correction + fixed_prediction);
      #else
        int prediction = fixed_prediction;
      #endif

      int error = channel_value - prediction;
      
      int acc_inv_sign = (ctx_stats.acc > 0)? 1:0;
      error = acc_inv_sign^ctx_sign ? -error: error;
      int q_error ;

      #if 0
        UQ_n_reduce(error,delta,q_idx,q_error);
      #else
        q_error = acc_inv_sign ? - error: error;
        q_idx = error;
      #endif

      q_channel_value = clamp(prediction + (ctx_sign==1? -q_error:q_error));

     
      //update encoder variables 
        update_context(ctx_id,q_error,ctx_stats,prev_ctx_stats);

      // quantized_img.update(q_channel_value);
      symbols << q_idx;

  }
  
  // entropy_encoder(EE_OP_ENCODE_BUFFER); // encode and insert contents of buffers in file 
}



#endif

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
