/*
  Copyright 2021 Tobías Alonso, Autonomous University of Madrid

  This file is part of LOCO-ANS.

  LOCO-ANS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LOCO-ANS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with LOCO-ANS.  If not, see <https://www.gnu.org/licenses/>.


 */
#include "codec_core.h"

// #include "color_space_transforms.h"
#include "img_proc_utils.h"
#include "core_analysis_utils.h"
#include "context.h"
#include "ANS_coder.h"


// int prediction_errors[512]={0};

int INPUT_BPP=8;
int MAXVAL = pow(2,INPUT_BPP)-1;
int EE_REMAINDER_SIZE =  (INPUT_BPP-1);

void set_codec_parameters(int ibpp,int near){
  INPUT_BPP=ibpp;
  MAXVAL = (1 << ibpp) - 1;
  #if ERROR_REDUCTION
    EE_REMAINDER_SIZE =  (INPUT_BPP-1);
  #else
    EE_REMAINDER_SIZE =  (INPUT_BPP);
  #endif
   
}


  int UQ(int error, int delta, int near){
    if(error > 0){
      error = (near + error)/delta;
    }else{
      error = -(near - error)/delta;
    }
    return error;
  }


  int predict(int a, int b, int c){
    int dx, dy, dxy, s;
    dy = a - c;
    dx = c -b;
    dxy = a -b;
    s = (dy ^ dx)<0? -1 : 0 ;
    dxy &= (dy ^ dxy)<0? -1 : 0 ;
    return  !s ? b + dy: a - dxy;
  
  }

  
  inline void get_prediction_and_context(RowBuffer &row_buffer,int col, int MAXVAL,
                                Context_t &context,int &prediction ){
    #if ADD_GRAD_4
      int a,b,c,d,e;
      row_buffer.get_teplate(col,a,b,c,d,e);
    #else
      int a,b,c,d;
      row_buffer.get_teplate(col,a,b,c,d);
    #endif

    // compute fix prediction
    int dy = c - a;
    int dx = b - c;
    int dxy = b - a ;
    int s = (dy ^ dx)>>(sizeof(s)*8-1) ;
    // int s = (dy ^ dx)<0? -1 : 0 ;
    dxy &= (dy ^ dxy)>>(sizeof(dxy)*8-1) ;
    // dxy &= (dy ^ dxy)<0? -1 : 0 ;
    int fixed_prediction = !s ? b - dy: a + dxy;

    // get context
    #if ADD_GRAD_4
      context = map_gradients_to_int(d - b, dx, dy,a-e);
    #else
      context = map_gradients_to_int(d - b, dx, dy);
    #endif

    //correct prediction
    prediction = clamp(get_context_bias(context) + fixed_prediction,MAXVAL);
  }
/*

*##################   Encoder  ########################
*/



  size_t image_scanner(const cv::Mat& src,uint8_t* binary_file,int near, int  &geometric_coder_iters, bool analysis_enabled = false){
    const int delta = 2*near +1;
    const int alpha = near ==0?MAXVAL + 1 :
                       (MAXVAL + 2 * near) / delta + 1;
    #if ERROR_REDUCTION
    const int MIN_REDUCT_ERROR = -near;
    const int MAX_REDUCT_ERROR =  MAXVAL + near;
    const int DECO_RANGE = alpha * delta;     
    const int MAX_ERROR =  std::ceil(alpha/2.0) -1;
    const int MIN_ERROR = -std::floor(alpha/2.0);
    #endif
    const int remainder_reduct_bits = std::floor(std::log2(float(delta)));

    
    context_init( near, alpha);
    Symbol_Coder symbol_coder(binary_file,EE_REMAINDER_SIZE);

    RowBuffer row_buffer(src.cols);
    
    //analysis
      theoretical_bits = 0;
      theoretical_entropy = 0;

    // store first px 
    {
      int channel_value = get_value(src,0,0); 
      #if DEBUG
        printf("First channel_value: %0X\n",channel_value );
      #endif
      symbol_coder.store_pixel(channel_value,INPUT_BPP);
      #ifdef ANALYSIS_CODE
        theoretical_bits +=INPUT_BPP;
        theoretical_entropy +=INPUT_BPP;
      #endif
      row_buffer.update(channel_value,0);
    }


    int init_col = 1;
    if(near == 0) { 
      // lossless coding: same algorithm, with some simplifications given that
      // near == 0, no division is required
      for (int row = 0; row < src.rows; ++row){
        row_buffer.start_row();
        const uchar * const row_ptr =  src.ptr<uchar>(row);
        for (int col = init_col; col < src.cols; ++col){
          int channel_value = row_ptr[col];
          int prediction;
          Context_t context;
          get_prediction_and_context(row_buffer,col, MAXVAL,context,prediction);

          int error = channel_value - prediction;
          
          int acc_inv_sign = (ctx_acc[context.id] > 0)? -1:0;
          error = mult_by_sign(error,context.sign^acc_inv_sign);

          #if ERROR_REDUCTION
            error -= MIN_ERROR;
            error &= MAXVAL;
            error += MIN_ERROR;
          #endif

          ee_symb_data symbol;
            symbol.y = error <0? 1:0;
            symbol.z = abs(error)-symbol.y;
            symbol.theta_id = get_context_theta_idx(context);
            symbol.p_id = ctx_p_idx[context.id];
            symbol.remainder_reduct_bits = remainder_reduct_bits;

          // get decoded value
          int q_error =  mult_by_sign(error,acc_inv_sign); 
          int q_channel_value = channel_value; // given that we are in lossless

          //update context
          row_buffer.update(q_channel_value,col);
          update_context(context, q_error,symbol.z,symbol.y);
      
          // entropy encoding
          symbol_coder.push_symbol(symbol);

          #ifdef ANALYSIS_CODE
            if(unlikely(analysis_enabled)) {
              estimate_entropy(symbol,context,near);
              estimate_code_length(symbol,context);
            }
          #endif
        }
        init_col = 0;
        row_buffer.end_row();
      }
    }else{
      #if USING_DIV_RED_LUT
      int16_t _div_reduct_lut[511];
      int16_t *div_reduct_lut = _div_reduct_lut+255;
      //init lut
        for(int error = -255; error <= 255; ++error) {
          int qerror = UQ(error, delta,near );
          if(unlikely(qerror < MIN_ERROR)){
            qerror += alpha;
          }else if(unlikely(qerror > MAX_ERROR)){
            qerror -= alpha;
          }
          *(div_reduct_lut+error) = qerror;
        }
      #endif

      for (int row = 0; row < src.rows; ++row){
      row_buffer.start_row();
      const uchar * const row_ptr =  src.ptr<uchar>(row);
      for (int col = init_col; col < src.cols; ++col){
        int channel_value = row_ptr[col];
        int prediction;
        Context_t context;
        get_prediction_and_context(row_buffer,col, MAXVAL,context,prediction);
        
        int error = channel_value - prediction;
        int acc_inv_sign = (ctx_acc[context.id] > 0)? -1:0;
        error = mult_by_sign(error,context.sign^acc_inv_sign);

        #if USING_DIV_RED_LUT
          error = *(div_reduct_lut+error) ;
        #else
          error = UQ(error, delta,near ); // quantize
          #if ERROR_REDUCTION
            if(unlikely(error < MIN_ERROR)){
              error += alpha;
            }else if(unlikely(error > MAX_ERROR)){
              error -= alpha;
            }
          #endif
        #endif

        ee_symb_data symbol;
          symbol.y = error <0? 1:0;
          symbol.z = abs(error)-symbol.y;
          symbol.theta_id = get_context_theta_idx(context);
          symbol.p_id = ctx_p_idx[context.id];
          symbol.remainder_reduct_bits = remainder_reduct_bits;
        

        // get decoded value
        int q_error = mult_by_sign(delta*error,acc_inv_sign); 
        int q_channel_value = (prediction + mult_by_sign(q_error,context.sign));

        #if ERROR_REDUCTION
          if(unlikely(q_channel_value < MIN_REDUCT_ERROR)){
            q_channel_value += DECO_RANGE;
          }else if(unlikely(q_channel_value > MAX_REDUCT_ERROR)){
            q_channel_value -= DECO_RANGE;
          }
        #endif 

        //update context
        q_channel_value = clamp(q_channel_value,MAXVAL);
        assert(abs(q_channel_value-channel_value)<=near);

        row_buffer.update(q_channel_value,col);
        update_context(context, q_error,symbol.z,symbol.y);


        // entropy encoding
          #ifdef ANALYSIS_CODE
          if(unlikely(analysis_enabled)) {
            estimate_entropy(symbol,context,near);
            estimate_code_length(symbol,context);
          }
          #endif
    
        symbol_coder.push_symbol(symbol);
      }
      init_col = 0;
      row_buffer.end_row();
    }
    }

    symbol_coder.code_symbol_buffer(); // encode and insert contents of buffers in file
    geometric_coder_iters = symbol_coder.get_geometric_coder_iters();
    return symbol_coder.get_out_file_size();
  }



  uint32_t encode_core(const cv::Mat& src,cv::Mat & quant_img,uint8_t* binary_file, char chroma_mode,
    char _fixed_prediction_alg, int near, char encoder_mode,int ibpp){
    // param setting and init

      if(chroma_mode != CHROMA_MODE_GRAY) {
        std::cerr<< "chroma_mode != CHROMA_MODE_GRAY.";
        std::cerr<< " Only gray images are currently supported ";
        std::cerr<<std::endl;
        //Other modes are not currently supported 
        throw 1;
      }

      if(ibpp > 8) {
        std::cerr<< "ibpp > 8 |";
        std::cerr<< " Only images 8 or less input bit per pixel currently supported ";
        std::cerr<<std::endl;;
        //Other modes are not currently supported 
        throw 1;
      }

      #ifdef DEBUG
      if((get_num_of_symbs(src.rows,src.cols,chroma_mode) % EE_BUFFER_SIZE) != 0) {
        std::cerr<<"Warning: possible codification inefficiency due to codification block misalign \n";
      }
      #endif

      set_codec_parameters(ibpp,near);


    //encode
      if(src.channels()== 3 ){ //I assume it's RGB image 
        std::cerr<<DBG_INFO<<"Only single channel images are supported"<<std::endl;
        throw 1;
      }
      // cv::Mat encoder_input_img;
      // if(src.channels()== 3 ){ //I assume it's RGB image 
      //   rgb2yuv(src, encoder_input_img,chroma_mode);
      // }else{
      //   encoder_input_img = src; //shallow copy
      // }

      bool analysis_enabled = (encoder_mode !=0) ;
      int geometric_coder_iters;

      uint32_t file_size = image_scanner(src,binary_file,near, geometric_coder_iters,analysis_enabled);

    #if DEBUG
      if(WARN_MAX_ST_IDX_cnt >0) {
        std::cerr<<"Codec Config: Warning: St idx > Max idx. Clamp percent: "<<
            float(WARN_MAX_ST_IDX_cnt)/(src.rows*src.cols)*100<<"%"<<std::endl;
      }
    #endif
    //output analysis
    #ifdef ANALYSIS_CODE
    if(analysis_enabled) {
      float num_symb = get_num_of_symbs(src.rows,src.cols,chroma_mode);

      printf("E= %.4Lf , Estim. bpp= %.4Lf , bpp= %.4lf , Avg iters= %.4lf ,\n",
                                  theoretical_entropy/num_symb, 
                                  theoretical_bits/num_symb,
                                  file_size*8.0/num_symb,
                                  float(geometric_coder_iters)/num_symb);
    }
    #endif
    return file_size;
  }


/*
*##################   Decoder  ########################
*/

  void binary_scanner(unsigned char* block_binary,cv::Mat& decoded_img,int near){
    //set run parameters
      const int delta = 2*near +1;
      const int alpha = near ==0? MAXVAL + 1 :
                       (MAXVAL + 2 * near) / delta + 1;
      const uint bit_reduction = std::floor(std::log2(delta));
      const uint escape_bits = EE_REMAINDER_SIZE - bit_reduction; 

      #if ERROR_REDUCTION
        const int DECO_RANGE = alpha * delta;
        const int MIN_REDUCT_ERROR = -near;
        const int MAX_REDUCT_ERROR =  MAXVAL + near;
      #endif 

    int num_of_symbols = get_num_of_symbs(decoded_img.rows,decoded_img.cols,CHROMA_MODE_GRAY);
    Binary_Decoder bin_decoder(block_binary,num_of_symbols);
    RowBuffer row_buffer(decoded_img.cols);

    //variable init 
      context_init( near, alpha);

    {
      int channel_value = bin_decoder.retrive_pixel(INPUT_BPP);
      row_buffer.update(channel_value,0);
      #if DEBUG
        printf("First channel_value: %0X\n",channel_value );
      #endif
      set_value(decoded_img,0,0,channel_value);
    }

    int init_col = 1;
    if(near == 0) {
      for (int row = 0; row < decoded_img.rows; ++row){
        row_buffer.start_row();
        uchar * const row_ptr =  decoded_img.ptr<uchar>(row);
        for (int col = init_col; col < decoded_img.cols; ++col){
          
          int prediction;
          Context_t context;
          get_prediction_and_context(row_buffer,col, MAXVAL,context,prediction);

           // entropy decoding
          int z,y,q_error;
          bin_decoder.retrive_TSG_symbol(get_context_theta_idx(context),ctx_p_idx[context.id],escape_bits,z,y);

          int error = y ==1? -z -1:z;
          q_error = error;
          q_error = (ctx_acc[context.id] > 0)?-q_error:q_error;
          int deco_val = (prediction + mult_by_sign(q_error,context.sign));
        
          #if ERROR_REDUCTION 
            deco_val &= MAXVAL;
          #endif 

          int q_channel_value = deco_val;
          row_buffer.update(q_channel_value,col);
          update_context(context, q_error,z,y);
          
          // store in output image
          row_ptr[col] = q_channel_value;
        }

        row_buffer.end_row();
        init_col= 0;
      }
    }else{
      for (int row = 0; row < decoded_img.rows; ++row){
        row_buffer.start_row();
        uchar * const row_ptr =  decoded_img.ptr<uchar>(row);
        for (int col = init_col; col < decoded_img.cols; ++col){
          
          int prediction;
          Context_t context;
          get_prediction_and_context(row_buffer,col, MAXVAL,context,prediction);

           // entropy decoding
          int z,y,q_error;
          bin_decoder.retrive_TSG_symbol(get_context_theta_idx(context),ctx_p_idx[context.id],escape_bits,z,y);

          int error = y ==1? -z -1:z;
          q_error = error*delta;
          q_error = (ctx_acc[context.id] > 0)?-q_error:q_error;
          int deco_val = (prediction + mult_by_sign(q_error,context.sign));
        
          #if ERROR_REDUCTION 
            if(unlikely(deco_val < MIN_REDUCT_ERROR)){
              deco_val += DECO_RANGE;
            }else if(unlikely(deco_val > MAX_REDUCT_ERROR)){
              deco_val -= DECO_RANGE;
            }
          #endif 

          int q_channel_value = clamp(deco_val,MAXVAL);
          row_buffer.update(q_channel_value,col);
          update_context(context, q_error,z,y);
          
          // store in output image
          row_ptr[col] = q_channel_value;
        }

        row_buffer.end_row();
        init_col= 0;
      }
    }

  }


  void decode_core(unsigned char* in_file ,cv::Mat& decode_img,char chroma_mode,
    char _fixed_prediction_alg , int near , uint ee_buffer_size, 
    int ibpp, char encoder_mode){


    if(chroma_mode != CHROMA_MODE_GRAY) {
      std::cerr<< "chroma_mode != CHROMA_MODE_GRAY.";
      std::cerr<< " Only gray images are currently supported ";
      std::cerr<<std::endl;
      //Other modes are not currently supported 
      throw 1;
    }

    if(ibpp > 8) {
      std::cerr<< "ibpp > 8 |";
      std::cerr<< " Only images 8 or less input bit per pixel currently supported ";
      std::cerr<<std::endl;;
      //Other modes are not currently supported 
      throw 1;
    }
    set_codec_parameters(ibpp,near);

    binary_scanner(in_file,decode_img,near);

  }



