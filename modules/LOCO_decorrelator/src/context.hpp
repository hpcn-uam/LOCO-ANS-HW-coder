
#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "LOCO_decorrelator.hpp"





struct ContextElement {
  ctx_cnt_t cnt;
  ctx_bias_t bias;
  ctx_acc_t acc;
  ctx_Nt_t Nt;
  ctx_p_idx_t p_idx;
  ctx_St_t St;
};

class Context
{
public:
  Context();
  ~Context();

};

ContextElement context_stats[CTX_GRAD_BINS];

struct quant_reduct_lut_elem{
  ap_int<INPUT_BPP> reduct_error;
  ap_int<INPUT_BPP+1> reconstruct_error;
};

constexpr int QUANT_RED_LUT_SIZE = (1<<(INPUT_BPP+1));
quant_reduct_lut_elem quant_reduct_lut[QUANT_RED_LUT_SIZE];

ap_int<INPUT_BPP+1> quant_error_lut[QUANT_RED_LUT_SIZE];

int Uniform_quantizer(int error, int delta, int near){
  if(error > 0){
    error = (near + error)/delta;
  }else{
    error = -(near - error)/delta;
  }
  return error;
}





#ifdef LOCO_DECORRELATOR_LS_TOP
void init_context(int near, int alpha){
  #pragma HLS inline
  const int ctx_initial_Nt = 0;
  const int ctx_initial_p_idx = MAX(CTX_NT_HALF_IDX>>1 ,CTX_NT_HALF_IDX -2 -near);
  const int ctx_initial_St = MAX(2, ((alpha + 32) >> 6 ))<<CTX_ST_PRECISION;


  init_ctx_loop: for(unsigned i = 0; i < CTX_GRAD_BINS; ++i) {
    // #pragma HLS unroll factor=2
    // I can unroll x2 switching from s2p_ram to t2p_ram
    // This comes at the cost of doubling the number of BRAMs (if these are used)
    // as 18K BRAMS in s2p mode can be configured as 36x512 but in t2p the
    //  wider config is 18x1028.
    // CTX_GRAD_BINS = 365 for T=4 and 3 grads.

    context_stats[i].cnt = 1;
    context_stats[i].bias = 0;
    context_stats[i].acc = 0;
    context_stats[i].Nt = ctx_initial_Nt;
    context_stats[i].p_idx = ctx_initial_p_idx;
    context_stats[i].St= ctx_initial_St;

  }
}
#else
void init_context(int near, int alpha){
  #pragma HLS inline
  const int ctx_initial_Nt = 0;
  const int ctx_initial_p_idx = std::max(CTX_NT_HALF_IDX>>1 ,CTX_NT_HALF_IDX -2 -near);
  const int ctx_initial_St = std::max(2, ((alpha + 32) >> 6 ))<<CTX_ST_PRECISION;

  const int delta = (near <<1) +1;
  const int DECO_RANGE = alpha * delta;
  const int MAX_ERROR =  (alpha+1)/2 -1; //  std::ceil(alpha/2.0) -1;
  const int MIN_ERROR = -(alpha/2); // -std::floor(alpha/2.0);

  constexpr int LOOP_ITERS = MAX(CTX_GRAD_BINS,QUANT_RED_LUT_SIZE);
  init_ctx_loop: for(int i = 0; i < LOOP_ITERS; ++i) {
  // init_ctx_loop: for(unsigned i = 0; i < CTX_GRAD_BINS; ++i) {
    // #pragma HLS unroll factor=2
    // I can unroll x2 switching from s2p_ram to t2p_ram
    // This comes at the cost of doubling the number of BRAMs (if these are used)
    // as 18K BRAMS in s2p mode can be configured as 36x512 but in t2p the
    //  wider config is 18x1028.
    // CTX_GRAD_BINS = 365 for T=4 and 3 grads.

    if(i < CTX_GRAD_BINS) {
      context_stats[i].cnt = 1;
      context_stats[i].bias = 0;
      context_stats[i].acc = 0;
      context_stats[i].Nt = ctx_initial_Nt;
      context_stats[i].p_idx = ctx_initial_p_idx;
      context_stats[i].St= ctx_initial_St;
    }

    if (i < QUANT_RED_LUT_SIZE){
      int orig_error = i - MAXVAL;
      ap_uint<INPUT_BPP+1> lut_address = orig_error;

      int error = Uniform_quantizer(orig_error,delta,near);
      quant_error_lut[lut_address] = error*delta- orig_error;
      if((error < MIN_ERROR)){
        error += alpha;
      }else if((error > MAX_ERROR)){
        error -= alpha;
      }
      quant_reduct_lut[lut_address].reduct_error = error;
      quant_reduct_lut[lut_address].reconstruct_error = error*delta;
    }

  }
}
#endif

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

  #elif 0
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
  #else

  static const int grad_quant_lut[64]={
    0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -2, -2, -2, -2, -1, -1
  };
  #pragma HLS ARRAY_PARTITION variable=grad_quant_lut complete dim=0
  // #pragma HLS bind_storage variable=grad_quant_lut type=ROM_1P latency=0
  q = grad_quant_lut[ap_uint<6>(g)];
  q = g >=32? 4: (g<=-32?-4:q );
  return q;
  #endif
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
  int y,
  int z,
  ContextElement current_stats,
  ContextElement &updated_stats){
  #pragma HLS inline

  int acc = current_stats.acc;
  // ap_int<3+CTX_ADJUST_CNT_BITS>  acc = current_stats.acc;
  // auto & acc = current_stats.acc;
  // ap_uint<1+CTX_ADJUST_CNT_BITS> cnt = current_stats.cnt;
  uint cnt = current_stats.cnt;
  // auto & cnt = current_stats.cnt;
  auto & bias = current_stats.bias;
  auto & Nt = current_stats.Nt;
  auto & p_idx = current_stats.p_idx;
  auto & St = current_stats.St;

  if((context != CTX_0)) {
    acc += error;
  }
  Nt += y<<CTX_NT_PRECISION;
  St += (z<<CTX_ST_PRECISION );
  cnt++;

  //update_context_divisions

  // update limits // rounding range [-1/2, +1/2)
  const int Li =-((cnt+1)>>1); //ceil(cnt/2) // OPT: the +1 probably has no practical effect on compression
  // const int Li =-((cnt+1)>>1); //ceil(cnt/2) // OPT: the +1 probably has no practical effect on compression
  const int Ls = ((cnt)>>1); // floor(cnt/2)

  // update bias
    if ((Li >= acc)){
      if(bias >-128) {bias--;}
      acc += cnt;
      if((Li >= acc)){acc= Li+1; }
    } else if ((acc > Ls)){
      if(bias <127) {bias++;}
      acc -= cnt ;
      if ((acc > Ls)){ acc= Ls;}
    }

  //update p id
    ASSERT(CTX_NT_CENTERED_QUANT,==,true);
    ASSERT(HALF_Y_CODER,==,true);
    Nt -= p_idx;

    #if 0
      const int low_diff = Nt -Li;
      const int high_diff = Ls -Nt;
      const int low_mask =(low_diff)>>(sizeof(low_diff)*8 -1);
      const int high_mask =(high_diff)>>(sizeof(high_diff)*8 -1);
      #define MAX_Nt_IDX  CTX_NT_HALF_IDX-1
      if((p_idx <MAX_Nt_IDX)) {
        /* code */
        p_idx -= high_mask;
        Nt -= cnt&high_mask;
      }

      p_idx += low_mask;
      Nt += cnt&low_mask;

    #else

      if ((Li > Nt)){
        p_idx--;
        Nt += cnt;
      } else if ((Nt >= Ls)){
        #if HALF_Y_CODER
          #define MAX_Nt_IDX  CTX_NT_HALF_IDX-1//((1<<(CTX_NT_PRECISION))-1)// CTX_NT_HALF_IDX-1
          if ((p_idx <MAX_Nt_IDX)) {
            p_idx++;
            Nt -= cnt ;
          }
        #else
          p_idx++;
          Nt -= cnt ;
        #endif
      }

    #endif
    ASSERT(p_idx,>=,0);
    #if CTX_NT_CENTERED_QUANT
      ASSERT(p_idx,<=,(1<<CTX_NT_PRECISION));
    #else
      ASSERT(p_idx,<,(1<<CTX_NT_PRECISION));
    #endif

  if(cnt >= CTX_ADJUST_CNT ) {
    cnt >>=1;
    acc = (acc >= 0)? ctx_acc_t(acc >> 1): ctx_acc_t(-((1 - acc) >> 1));
    Nt  >>=1;
    St  >>=1;
  }

  current_stats.acc = acc;
  current_stats.cnt = cnt;
  updated_stats = current_stats;
  context_stats[context]= current_stats;

}

#endif // CONTEXT_HPP
