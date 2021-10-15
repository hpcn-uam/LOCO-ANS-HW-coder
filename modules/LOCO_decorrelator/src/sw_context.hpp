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

#ifndef CONTEXT_H
#define CONTEXT_H


#include "sw_implementation.hpp"
using namespace sw_impl;

#include <array>
#include <assert.h>
#include <math.h>
#include <algorithm>

#define CTX_DIMS 3

#define CTX_BINS_PER_DIM 9
constexpr int CTX_DIM_OFFSET = (CTX_BINS_PER_DIM-1)/2;

#define NEW_CTX_MAP 1
#if NEW_CTX_MAP
constexpr int  CTX_0 = 0;
  #if ADD_GRAD_4
    #define CTX_GRAD_BINS ((((2*CTX_DIM_OFFSET+1)*(2*CTX_DIM_OFFSET+1)*(2*CTX_DIM_OFFSET+1)+1)/2) *3) //343 // 729
    #define CTX_BINS_NON_COLAP (((2*CTX_DIM_OFFSET+1)*(2*CTX_DIM_OFFSET+1)*(2*CTX_DIM_OFFSET+1)))
    #define CTX_OFFSET ((CTX_BINS_NON_COLAP-1)/2)
  #else
    #define CTX_GRAD_BINS (((2*CTX_DIM_OFFSET+1)*(2*CTX_DIM_OFFSET+1)*(2*CTX_DIM_OFFSET+1)+1)/2 ) //(729) //343 // 729
    #define CTX_BINS_NON_COLAP (((2*CTX_DIM_OFFSET+1)*(2*CTX_DIM_OFFSET+1)*(2*CTX_DIM_OFFSET+1)))
    #define CTX_OFFSET ((CTX_BINS_NON_COLAP-1)/2)
  #endif
#else
  constexpr int  CTX_0 =(CTX_DIM_OFFSET*CTX_BINS_PER_DIM+CTX_DIM_OFFSET);
  #if ADD_GRAD_4
    #define CTX_GRAD_BINS ((((2*CTX_DIM_OFFSET+1)*(2*CTX_DIM_OFFSET+1)*(2*CTX_DIM_OFFSET+1)+1)/2) *3) //343 // 729
  #else
    #define CTX_GRAD_BINS (450) //343 // 729

  #endif

#endif
//const int CTX_GRAD_BINS = pow(CTX_BINS_PER_DIM,CTX_DIMS);


#define CTX_BINS (CTX_GRAD_BINS )

// Context state variables
std::array<int, CTX_BINS> ctx_cnt={0};

std::array<int, CTX_BINS> ctx_acc={0};
std::array<int, CTX_BINS> ctx_mean={0};

std::array<int, CTX_BINS> ctx_Nt={0};
std::array<int, CTX_BINS> ctx_p_idx={0};

std::array<int, CTX_BINS> ctx_St={0};
#if ! ITERATIVE_ST
std::array<int, CTX_BINS> ctx_St_idx={0};
#endif

#define CTX_MU_PRECISION 0  // number of fractional bits
#define CTX_MU_ACC_BITS 11  // number of bits
const int CTX_MU_FACTOR = int(pow(2,CTX_MU_PRECISION));
const int CTX_ACC_MAX_VAL = int(pow(2,CTX_MU_ACC_BITS-1)-1); // CTX_MU_ACC_BITS-1 because is a signed accumulator

#define CTX_NT_HALF_IDX (1<<(CTX_NT_PRECISION-1))

// #define MAX_Nt_IDX  CTX_NT_HALF_IDX-1//((1<<(CTX_NT_PRECISION))-1)// CTX_NT_HALF_IDX-1
const uint CTX_NT_FACTOR = uint(pow(2,CTX_NT_PRECISION)); // number of fractional bits


#define CTX_ST_BITS (11 +CTX_ST_PRECISION ) // number of bits

const uint CTX_ST_FACTOR = uint(pow(2,CTX_ST_PRECISION));
const uint CTX_ST_MAX_VAL = uint(pow(2,CTX_ST_BITS)-1);


// initial params
#define CTX_ADJUST_CNT 64
const int ctx_initial_cnt = 1;
const int ctx_initial_Nt = 0;




int sw_impl::gradient_quantizer(int g){
  int T0=0,T1=3,T2=7,T3=21;

  int sign = 1;
  if(g < 0){
    sign = -1;
    g = -g;
  }

  int q;
  // check from high to low probability
  if(g <= T0){
    q = 0;
  }else if (g < T1){
    q = 1*sign;
  }else if (g < T2){
    q = 2*sign;
  }else if (g < T3){
    q = 3*sign;
  #if CTX_BINS_PER_DIM < 11
    }else{
      q = 4*sign;
    }
  #else
    }else if (g < T4){
      q = 4*sign;
    }else{
      q = 5*sign;
    }
  #endif
  return q;
}

inline int mult_by_sign( int val, int sign) {
  return (sign ^ val) - sign;
}

 int8_t _gradient_quant[256*2],*gradient_quant;

Context_t map_gradients_to_int(int g1, int g2, int g3){
  #if 1
  int q1 = gradient_quantizer(g1);
  int q2 = gradient_quantizer(g2);
  int q3 = gradient_quantizer(g3);
  #else
  int q1  = *(gradient_quant+g1);
  int q2  = *(gradient_quant+g2);
  int q3  = *(gradient_quant+g3);
  #endif

  int context_id = (q1*CTX_BINS_PER_DIM + q2 )*CTX_BINS_PER_DIM + q3 ;

  int sign = context_id >> (sizeof(context_id)*8 - 1);
  context_id = mult_by_sign(context_id, sign);

  assert(context_id<CTX_BINS);
  assert(0<=context_id);
  assert(!(q1==0 && q2==0 && q3==0 ) ||(CTX_0 ==context_id ));

  return Context_t(context_id,sign);
}

int grad4_quant(int g){
  constexpr int T4 = 5;
  if( g <= -T4){
    return -1;
  }else if( g < T4){
    return 0;
  }else{
    return 1;
  }
}

int8_t _gradient4_quant[256*2],*gradient4_quant;

Context_t map_gradients_to_int(int g1, int g2, int g3, int g4){
  int q1  = *(gradient_quant+g1);
  int q2  = *(gradient_quant+g2);
  int q3  = *(gradient_quant+g3);
  int q4 = *(gradient4_quant+g4);

  int context_id = ((q1*CTX_BINS_PER_DIM + q2 )*CTX_BINS_PER_DIM + q3)*3 + q4 ;
  int sign = context_id >> (sizeof(context_id)*8 - 1);
  context_id = mult_by_sign(context_id, sign);

  assert(context_id<CTX_BINS);
  assert(0<=context_id);
  assert(!(q1==0 && q2==0 && q3==0 && q4==0 ) ||(CTX_0 ==context_id ));

  return Context_t(context_id,sign);
}



inline int get_context_bias(Context_t context){
  #if CTX_MU_PRECISION == 0
    return mult_by_sign(ctx_mean[context.id],context.sign);
  #else
    const int abs_bias = (CTX_MU_FACTOR/2)-1;
    int bias = ctx_mean[context.id]>0? abs_bias:-abs_bias;
    return mult_by_sign(((ctx_mean[context.id]+bias)/CTX_MU_FACTOR),context.sign);
  #endif
}

#if DEBUG
static long WARN_MAX_ST_IDX_cnt = 0;
#endif


int get_st_idx(Context_t context){
  int idx;
  #if CTX_ST_FINER_QUANT
    int e, l = ctx_cnt[context.id];
    for(e = 0; ctx_St[context.id] > l; l<<=1,e+=2) {;}
    // idx = e<<1;
    idx = e;
    if(ctx_St[context.id]> l-((l+2)>>2)){
      idx++;
    }
  #else
    for(idx = 0; ctx_St[context.id] > (ctx_cnt[context.id]<<(idx)); ++idx) {;}
  #endif


  idx = idx>MAX_ST_IDX?MAX_ST_IDX:idx;


  return idx;
}





inline int get_context_theta_idx(Context_t context){
  #if ITERATIVE_ST
    return get_theta_idx( ctx_cnt[context.id],ctx_St[context.id] );
  #else
    return ctx_St_idx[context.id];
  #endif
}




void context_init(int near, int alpha){
  //variable init
  for(auto & val: ctx_cnt){val=ctx_initial_cnt;}
  for(auto & val: ctx_acc){val=0;}
  for(auto & val: ctx_mean){val=0;}
  #if MU_estim_like_original
    int ctx_initial_p_idx = CTX_NT_HALF_IDX;
  #else
    int ctx_initial_p_idx = std::max(CTX_NT_HALF_IDX>>1 ,CTX_NT_HALF_IDX -2 -near);
  #endif
  for(auto & val: ctx_p_idx){val=ctx_initial_p_idx;}
  for(auto & val: ctx_Nt){val=ctx_initial_Nt;}

  const int ctx_initial_St = std::max(2, ((alpha + 32) >> 6 ))<<CTX_ST_PRECISION;
  for(auto & val: ctx_St){val=ctx_initial_St;}
  #if ! ITERATIVE_ST
    const int ctx_initial_St_idx =  get_st_idx(Context_t());
    for(auto & val: ctx_St_idx){val=ctx_initial_St_idx;}
  #endif


  gradient_quant = _gradient_quant+255;
  gradient4_quant = _gradient4_quant+255;

  for(int i = -255; i < 256; ++i) {
    _gradient_quant[i+255] = gradient_quantizer(i);
    *(gradient4_quant+i) = grad4_quant(i);
  }


}

void update_context(Context_t ctx, int prediction_error,int z, int y){
  int context = ctx.id;
  auto & cnt = ctx_cnt[context];
  auto & acc = ctx_acc[context];
  auto & mean = ctx_mean[context];
  auto & Nt = ctx_Nt[context];
  auto & p_idx = ctx_p_idx[context];
  auto & St = ctx_St[context];
  #if !ITERATIVE_ST
  auto & St_idx = ctx_St_idx[context];
  #endif
  //update accumulators and counters
    // ctx_acc[context] += context == CTX_0 ? 0: prediction_error<<CTX_MU_PRECISION; // CTX_0 shouldn't have bias
    if((context != CTX_0)) {
      acc += prediction_error<<CTX_MU_PRECISION;
    }
    Nt += y<<CTX_NT_PRECISION;
    St += (z<<CTX_ST_PRECISION );
    cnt++;

  //update_context_divisions

  // update limits // rounding range [-1/2, +1/2)
  const int Li =-((cnt+1)>>1); //ceil(cnt/2) // OPT: the +1 probably has no practical effect on compression
  const int Ls = ((cnt)>>1); // floor(cnt/2)
    //mu
    {
      #if MU_estim_like_original
        int Li =-cnt; //ceil(cnt/2)
        int Ls = 0; // floor(cnt/2)
      #endif

      //TODO limit mu values to decide it's range, and in consequence it's bits
      if ((Li >= acc)){
        if(mean >-128) {mean--;}
        acc += cnt;
        if((Li >= acc)){acc= Li+1; }
      } else if ((acc > Ls)){
        if(mean <127) {mean++;}
        acc -= cnt ;
        if ((acc > Ls)){ acc= Ls;}
      }
    }

    //Nt
    {
      #if ! CTX_NT_CENTERED_QUANT
        const int Li = 0 ;
        const int Ls = cnt;
      #endif

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
      assert(p_idx>=0);
      #if CTX_NT_CENTERED_QUANT
        assert(p_idx<=(1<<CTX_NT_PRECISION));
      #else
        assert(p_idx<(1<<CTX_NT_PRECISION));
      #endif
    }

  #if ! ITERATIVE_ST
    {

      #if CTX_ST_FINER_QUANT
        int idx = St_idx>>1;
        int Li = idx? cnt<< (idx-1) : 0;
        int Ls = cnt<< idx;

        St_idx &= 0xFFFFE;
        if ((Li >= St && St_idx)){
          // St_idx >=2 at this point
          St_idx-=2;
        } else if ((St > Ls)){
          #if DEBUG
            if(St_idx == MAX_ST_IDX) {
              WARN_MAX_ST_IDX_cnt++;
            }
          #endif
          if((St_idx <MAX_ST_IDX-1  )) {
            St_idx+=2;
          }
        }

        // Next code implements:
        //  if((St> Ls-((Ls+2)>>2))){
        //    if(St_idx <MAX_ST_IDX  ) {St_idx |=1;}
        //  }else{
        //    St_idx &= 0xFFFFE;
        //  }
        //
        // Note: (needs the "St_idx &= 0xFFFFE;" line used earlier)

        int hf_lim = (Ls-((Ls+2)>>2));
        int hf_idx = (hf_lim- St)>>(sizeof(hf_idx)*8-1);// -1 if (St> Ls-((Ls+2)>>2)) else 0

        #if MAX_ST_IDX %2 != 0
          St_idx |= -hf_idx;
        #else
          if(St_idx <MAX_ST_IDX  ) St_idx |= - hf_idx;
        #endif


        assert(St_idx >=0);
        assert(St_idx <=MAX_ST_IDX);

      #else
        const int Li = ((St_idx)) ? cnt<< (St_idx-1):0;
        const int Ls = cnt<< St_idx;

        if ((Li > St)){
          St_idx--;
          if(((Li>>1) > St)){
            St_idx--;
            // if(((Li>>2) > St)){St_idx--;}
          }
        } else if ((St >= Ls)){
          #if DEBUG
            if(St_idx == MAX_ST_IDX) {
              WARN_MAX_ST_IDX_cnt++;
            }
          #endif

          if((St_idx <MAX_ST_IDX ) ) {
            St_idx++;
           /* if ((St >= (Ls<<1))){
              if(St_idx <MAX_ST_IDX  ) {
                St_idx++;
              }
            }*/
          }
        }

      #endif

    }
  #endif

  // adjust
    if((cnt >= CTX_ADJUST_CNT )) {
      cnt >>=1;
      acc = (acc >= 0)? (acc >> 1): ((1 + acc) >> 1);
      Nt  >>=1;
      St  >>=1;
    }

}



#endif // CONTEXT_H
