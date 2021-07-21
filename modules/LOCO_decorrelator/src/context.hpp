
#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "LOCO_decorrelator.hpp"


constexpr int CTX_0 = 0;
constexpr int CTX_BINS_PER_DIM =9;
constexpr int CTX_GRAD_BINS =(((CTX_BINS_PER_DIM)*(CTX_BINS_PER_DIM)*(CTX_BINS_PER_DIM)+1)/2);
constexpr int CTX_ADJUST_CNT_BITS   =6;
constexpr int CTX_ADJUST_CNT   =1<<CTX_ADJUST_CNT_BITS;

typedef  ap_int<8> ctx_bias_t;
// typedef  ap_int<10> ctx_bias_t;
// typedef  ap_uint<CTX_ADJUST_CNT_BITS+5> ctx_cnt_t;
typedef  ap_uint<CTX_ADJUST_CNT_BITS+1> ctx_cnt_t;
// typedef  ap_int<INPUT_BPP+CTX_ADJUST_CNT_BITS+5> ctx_acc_t;
typedef  ap_int<INPUT_BPP+CTX_ADJUST_CNT_BITS> ctx_acc_t;

struct ContextElement {
  ctx_cnt_t cnt;
  ctx_bias_t bias;
  ctx_acc_t acc;
};

class Context
{
public:
  Context();
  ~Context();
  
};

constexpr int NUM_OF_CTX = 512;
ContextElement context_stats[NUM_OF_CTX];
// ctx_cnt_t context_cnt[NUM_OF_CTX];
// ctx_bias_t context_bias[CTX_GRAD_BINS];
// ctx_acc_t context_acc[NUM_OF_CTX];

void init_context(){
  // #pragma HLS inline
  init_ctx_loop: for(unsigned i = 0; i < CTX_GRAD_BINS; ++i) {
    // context_bias[i] = 0;
    context_stats[i].cnt = 1;
    context_stats[i].bias = 0;
    context_stats[i].acc = 0;
  }
}


// inline ap_uint<4> gradient_quantizer(int g){
int gradient_quantizer(int g){
  #pragma HLS inline
  constexpr int T0=0,T1=3,T2=7,T3=21;
  
  int q;
  //OPT? using simetry
  // OPT implement like a tree
  // or both, first  check if it's  in the central threshold, if not remove sign and continue
  #if 0
  if (g <= -T3) {
    q = -4;
  } else if (g <= -T2) {
    q = -3;
  } else if (g <= -T1) {
    q = -2;
  } else if (g <  -T0) {
    q = -1;
  } else if (g <=  T0) {
    q = 0;
  } else if (g <   T1) {
    q = 1;
  } else if (g <   T2) {
    q = 2;
  } else if (g <   T3) {
    q = 3;
  } else {
    q = 4;
  }
  return  q;
  #elif 0

  int sign;
  if(g < 0){
    sign = 1;
    g = -g;
  }else{
    sign = 0;
  }
  if(g <= T0){
    q = 0;
  }else if (g < T1){
    q = 1;
  }else if (g < T2){
    q = 2;
  }else if (g < T3){
    q = 3;
    }else{
      q = 4;
    }
  
  return sign==0? q:-q;
  #elif 0

  int abs_g = abs(g);
  if(abs_g <= T0){
    q = 0;
  }else if (abs_g < T1){
    q = 1;
  }else if (abs_g < T2){
    q = 2;
  }else if (abs_g < T3){
    q = 3;
    }else{
      q = 4;
    }
  
  return g<0? -q:q;

  #else
    int abs_g = abs(g);

    if(abs_g < T2) {
      if (abs_g < T1){
        if(abs_g <=T0) {
          q=0;
        }else{
          q=1;
        }
      }else{
        q=2;
      }
    }else{
      if (abs_g < T3){
      q = 3;
      }else{
        q = 4;
      }
    }

  return g<0? -q:q;
  #endif
}

int gradient_quantizer_0(int g){
  #pragma HLS inline
  constexpr int T0=0,T1=3,T2=7,T3=21;
  
  int q;
  //OPT? using simetry
  // OPT implement like a tree
  // or both, first  check if it's  in the central threshold, if not remove sign and continue

  if (g <= -T3) {
    q = -4;
  } else if (g <= -T2) {
    q = -3;
  } else if (g <= -T1) {
    q = -2;
  } else if (g <  -T0) {
    q = -1;
  } else if (g <=  T0) {
    q = 0;
  } else if (g <   T1) {
    q = 1;
  } else if (g <   T2) {
    q = 2;
  } else if (g <   T3) {
    q = 3;
  } else {
    q = 4;
  }

  return q;
}

int gradient_quantizer_1(int g){
  #pragma HLS inline
  constexpr int T0=0,T1=3,T2=7,T3=21;
  
  int q;
  //OPT? using simetry
  // OPT implement like a tree
  // or both, first  check if it's  in the central threshold, if not remove sign and continue

  if (g <= -T3) {
    q = -4;
  } else if (g <= -T2) {
    q = -3;
  } else if (g <= -T1) {
    q = -2;
  } else if (g <  -T0) {
    q = -1;
  } else if (g <=  T0) {
    q = 0;
  } else if (g <   T1) {
    q = 1;
  } else if (g <   T2) {
    q = 2;
  } else if (g <   T3) {
    q = 3;
  } else {
    q = 4;
  }

  return q;
}

int gradient_quantizer_2(int g){
  #pragma HLS inline
  constexpr int T0=0,T1=3,T2=7,T3=21;
  
  int q;
  //OPT? using simetry
  // OPT implement like a tree
  // or both, first  check if it's  in the central threshold, if not remove sign and continue

  if (g <= -T3) {
    q = -4;
  } else if (g <= -T2) {
    q = -3;
  } else if (g <= -T1) {
    q = -2;
  } else if (g <  -T0) {
    q = -1;
  } else if (g <=  T0) {
    q = 0;
  } else if (g <   T1) {
    q = 1;
  } else if (g <   T2) {
    q = 2;
  } else if (g <   T3) {
    q = 3;
  } else {
    q = 4;
  }

  return q;
}



inline void map_gradients_to_int(int g1, int g2, int g3,int &context,int &sign){
  #pragma HLS inline
  int q1 = gradient_quantizer(g1);
  int q2 = gradient_quantizer(g2);
  int q3 = gradient_quantizer(g3);

  #if 0
  if (q1 < 0 || (q1 == 0 && q2 < 0) || (q1 == 0 && q2 == 0 && q3 < 0)) {
    q1       = -q1;
    q2       = -q2;
    q3       = -q3;
    sign = -1;
  } else {
    sign = 0;
  }
  // context = q1 * 9 * 9 + (q2 ) * 9 + (q3 ) ;
  context = q1 * 9 * 9 + (q2 + 4) * 9 + (q3 + 4) ;

  #else
  // this alternative results in better hw: lower resource, and less deep pipeline.
  context = (q1*CTX_BINS_PER_DIM + q2 )*CTX_BINS_PER_DIM;
  context += q3 ;
  if(context<0) {
    sign =1;
    context = -context;
  }else{
    sign =0;
  }
  // int sign = context >> (sizeof(context)*8 - 1);
  #endif
}


/*int get_context_bias(){

}*/

inline void update_context(
  int context,
  int error,
  ContextElement current_stats,
  ContextElement &updated_stats){
  #pragma HLS inline

  auto & acc = current_stats.acc;
  auto & cnt = current_stats.cnt;
  auto & bias = current_stats.bias;

  if((context != CTX_0)) {
    acc += error;
  }
  // Nt += y<<CTX_NT_PRECISION;
  // St += (z<<CTX_ST_PRECISION );
  cnt++;

  //update_context_divisions

  // update limits // rounding range [-1/2, +1/2)
  const int Li =-((cnt+1)>>1); //ceil(cnt/2) // OPT: the +1 probably has no practical effect on compression
  const int Ls = ((cnt)>>1); // floor(cnt/2)

  if ((Li >= acc)){
    bias--;
    acc += cnt;
    if((Li >= acc)){acc= Li+1; }
  } else if ((acc > Ls)){
    bias++;
    acc -= cnt ;
    if ((acc > Ls)){ acc= Ls;}
  }
  
  if(cnt >= CTX_ADJUST_CNT ) { 
    cnt >>=1; 
    acc >>=1;  
    // Nt  >>=1;
    // St  >>=1;
  }    

  updated_stats.bias = bias;
  updated_stats.cnt = cnt;
  updated_stats.acc = acc;

  // updated_bias =bias;
  // context_stats[context].cnt=cnt ;
  // context_stats[context].bias= updated_bias;
  // context_stats[context].acc= acc;
  context_stats[context]= updated_stats;

}

#endif // CONTEXT_HPP
