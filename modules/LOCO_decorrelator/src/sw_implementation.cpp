/*==================================================================================================
*-----------------------------------------------------------------------------
*Copyright (c) %Y. All rights reserved
*-----------------------------------------------------------------------------
* @Filename      : sw_implementation.cpp
* @Author        : Tobías Alonso
* @Email         : talonsopugliese@gmail.com
* @Created On    : 2021-07-15 14:33:55
* @Description   :
*
*
*
* Last Modified : 2021-07-22 11:01:48
*
* Revision      :
*
* Disclaimer
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*==================================================================================================
*/

#include "sw_implementation.hpp"
#include "sw_context.hpp"
#include <math.h>
#include <assert.h>

using namespace sw_impl;
using namespace std;

#define SW_ERROR_REDUCTION 1
#define SW_USING_DIV_RED_LUT 0
// int prediction_errors[512]={0};

// constexpr int MAXVAL = (1<<INPUT_BPP)-1;
#define ADD_GRAD_4 0

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

 int sw_impl::get_theta_idx(int ctx_cnt, int ctx_St){
  int idx;
  #if CTX_ST_FINER_QUANT
    int e, l = ctx_cnt;
    for(e = 0; ctx_St > l; l<<=1,e+=2) {;}
    // idx = e<<1;
    idx = e;
    if(ctx_St> l-((l+2)>>2)){
      idx++;
    }
  #else
    for(idx = 0; ctx_St > (ctx_cnt<<(idx)); ++idx) {;}
  #endif

  #if DEBUG
    if(idx > MAX_ST_IDX) {
      WARN_MAX_ST_IDX_cnt++;
      idx = MAX_ST_IDX;
    }
  #else
    idx = idx>MAX_ST_IDX?MAX_ST_IDX:idx;
  #endif


  return idx;
}


inline unsigned int clamp(int v, unsigned MAXVAL = 255){  //cv::saturate_cast<uchar>
  return (unsigned int)((unsigned)v <= MAXVAL ? v : v > 0 ? MAXVAL : 0);
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
  // prediction =  fixed_prediction;
  prediction = clamp(get_context_bias(context) + fixed_prediction,MAXVAL);
}




void sw_impl::image_scanner(int near,int cols, int rows,
  hls::stream<int>& src, hls::stream<ee_symb_data>& symbols){
  const int delta = 2*near +1;
  const int alpha = near ==0?MAXVAL + 1 :
                     (MAXVAL + 2 * near) / delta + 1;
  #if SW_ERROR_REDUCTION
  const int MIN_REDUCT_ERROR = -near;
  const int MAX_REDUCT_ERROR =  MAXVAL + near;
  const int DECO_RANGE = alpha * delta;
  const int MAX_ERROR =  std::ceil(alpha/2.0) -1;
  const int MIN_ERROR = -std::floor(alpha/2.0);
  #endif
  const int remainder_reduct_bits = std::floor(std::log2(float(delta)));


  context_init( near, alpha);

  RowBuffer row_buffer(cols);

  // store first px
  {
    int channel_value = src.read();
    row_buffer.update(channel_value,0);
    ee_symb_data symbol;
    symbol.z = channel_value;
    symbols<< symbol;
  }


  int init_col = 1;
  #if SW_USING_DIV_RED_LUT
  int16_t _div_reduct_lut[511];
  int16_t *div_reduct_lut = _div_reduct_lut+255;
  //init lut
    for(int error = -255; error <= 255; ++error) {
      int qerror = UQ(error, delta,near );
      if((qerror < MIN_ERROR)){
        qerror += alpha;
      }else if((qerror > MAX_ERROR)){
        qerror -= alpha;
      }
      *(div_reduct_lut+error) = qerror;
    }
  #endif

  for (int row = 0; row < rows; ++row){
    row_buffer.start_row();
    // const uchar * const row_ptr =  src.ptr<uchar>(row);
    for (int col = init_col; col < cols; ++col){
      int channel_value = src.read();
      // int channel_value = row_ptr[col];
      int prediction;
      Context_t context;
      get_prediction_and_context(row_buffer,col, MAXVAL,context,prediction);

      int error = channel_value - prediction;

      int acc_inv_sign = (ctx_acc[context.id] > 0)? -1:0;
      error = mult_by_sign(error,context.sign^acc_inv_sign);

      // symbols<< error;

      #if 1
        #if SW_USING_DIV_RED_LUT
          error = *(div_reduct_lut+error) ;
        #else
          error = UQ(error, delta,near ); // quantize
          #if SW_ERROR_REDUCTION
            if((error < MIN_ERROR)){
              error += alpha;
            }else if((error > MAX_ERROR)){
              error -= alpha;
            }
          #endif
        #endif
      #endif

      ee_symb_data symbol;
        symbol.y = error <0? 1:0;
        symbol.z = abs(error)-symbol.y;
        symbol.theta_id = get_context_theta_idx(context);
        symbol.p_id = ctx_p_idx[context.id];
        symbol.remainder_reduct_bits = remainder_reduct_bits;

      symbols<< symbol;


      // get decoded value
      int q_error = mult_by_sign(delta*error,acc_inv_sign);
      int q_channel_value = (prediction + mult_by_sign(q_error,context.sign));

      #if SW_ERROR_REDUCTION
        if((q_channel_value < MIN_REDUCT_ERROR)){
          q_channel_value += DECO_RANGE;
        }else if((q_channel_value > MAX_REDUCT_ERROR)){
          q_channel_value -= DECO_RANGE;
        }
      #endif

      //update context
      q_channel_value = clamp(q_channel_value,MAXVAL);
      assert(abs(q_channel_value-channel_value)<=near);

      row_buffer.update(q_channel_value,col);
      update_context(context, q_error,symbol.z,symbol.y);



    }
    init_col = 0;
    row_buffer.end_row();
  }

}
