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

#ifndef CORE_ANALYSIS_UTILS_H
#define CORE_ANALYSIS_UTILS_H

#include "codec_core.h"
#include "img_proc_utils.h"
#include "context.h"
#include "ANS_coder.h"

std::array<double, CTX_BINS> ctx_code_bit_acc={0};

long double theoretical_bits = 0;
long double theoretical_entropy = 0;

float kld_minimizing_rec_value(float l,float h){
  const float C = log2(1-l) - log2(1-h) + l/(1-l)*log2(l) - h/(1-h)*log2(h);
  const float C1 = C/(l/(1-l)-h/(1-h));
  return pow(2,C1);
}

float get_St_rec_value(int idx, int context){
  float  low_b,high_b;
  const int p = -CTX_ST_PRECISION;

  if ((0x01&idx) >0){
    low_b = 3*pow(2,(idx>>1)-2 + p);
  }
  else{
    low_b = 1*pow(2,(idx>>1)-1 + p);
  }      

  int idx_h = idx +1;
  if ((0x01&idx_h) >0){
    high_b = 3*pow(2,(idx_h>>1)-2 + p);
  }
  else{
    high_b = 1*pow(2,(idx_h>>1)-1 + p);
  }

  assert(low_b<=high_b);
  float theta_rx = kld_minimizing_rec_value(low_b/(low_b+1),high_b/(high_b+1));
  float St_rx = theta_rx/(1-theta_rx);

  // verification 
  #if ITERATIVE_ST
    assert(idx ==0 || low_b<=(ctx_St[context]/float(CTX_ST_FACTOR))/((float) ctx_cnt[context]));
    assert((ctx_St[context]/float(CTX_ST_FACTOR))/((float) ctx_cnt[context])<=high_b+.01 || 
                idx == MAX_ST_IDX);
  #endif
  assert(low_b<=St_rx);
  assert(St_rx<=high_b);

  // return high_b;
  return St_rx;
}


void estimate_entropy(ee_symb_data symbol,Context_t context,int near  ){

  float t = (float) ctx_cnt[context.id];
  float St  = ctx_St[context.id]/float(CTX_ST_FACTOR);
  float Nt  =  (ctx_p_idx[context.id]*t + ctx_Nt[context.id])/float(CTX_NT_FACTOR);

  //y entropy
  double y_beta_a = .5;
  double y_beta_b = .5; 
  double p =  (Nt+ y_beta_a)/(t+ y_beta_b + y_beta_a);
  double y_prob = symbol.y == 1? p:1-p;

  // z entropy
  #if 0
    #if 0
      float z_prob = (t+.5)/(St+symbol.z+.5);
      for(int j = 0; j < symbol.z+1; ++j) {
        z_prob *= (St+j+.5)/(St+t+j+1.0);
      }
    #else
      double z_beta_a = .5/(1+near/2);//.5;
      double z_beta_b = .5;
      double z_prob = 1;
      for(int j = 0; j < symbol.z; ++j) {  
          z_prob *= (St+j+z_beta_a)/(St+j+t+ z_beta_a+z_beta_b)  ;
      }
      //code the one:  
      z_prob *= (t+z_beta_b)/(St+t+symbol.z+z_beta_a+z_beta_b );
    #endif
  #else
    double z_beta_a = .5/(1+near/2);//.5;
    double z_beta_b = .5;
    double theta = (St+ z_beta_a)/(St+t+ z_beta_a+z_beta_b);

    double z_prob = (1-theta)*pow(theta,symbol.z);
  #endif

  double symbol_prob = y_prob*z_prob;

  // compute entropy
    assert(symbol_prob <= 1 || symbol_prob >=0);
    symbol_prob = symbol_prob > 1e-20? symbol_prob : 1e-20; // to avoid log(0)

  theoretical_entropy  += -log2(symbol_prob);

}


void estimate_code_length(ee_symb_data symbol,Context_t context  ){

  // get reconstruction value (not done in coder, id select ANS table)
  // The reconstructions does not need to be in the center of the interval
  // as done here. The coder can be set to minimize the KLD for the quantization bin
  
  // y prob
  #if CTX_NT_CENTERED_QUANT
    double p = symbol.p_id ==0? 1/pow(2,(CTX_NT_PRECISION+2)) : symbol.p_id/pow(2,(CTX_NT_PRECISION)) ;
    if(symbol.p_id == (1<<CTX_NT_PRECISION)) { 
      p-=1/pow(2,(CTX_NT_PRECISION+2));
    }
  #else
  double p = pow(2,-(CTX_NT_PRECISION+1)) + symbol.p_id*pow(2,-CTX_NT_PRECISION);
  #endif
  double y_prob = symbol.y == 1? p:1-p;


  // z prob
  #if CTX_ST_FINER_QUANT
    float St_av = get_St_rec_value(symbol.theta_id,context.id);
  #else
    const float simp_ratio = 1/sqrt(2);
    float St_av = (1<<symbol.theta_id)*simp_ratio/CTX_ST_FACTOR;
  
  #endif

  float theta = (St_av)/(St_av+1);
  double z_prob = (1-theta)*pow(theta,symbol.z);


  double symbol_prob = y_prob*z_prob;

  // compute entropy
    assert(symbol_prob <= 1 || symbol_prob >=0);
    symbol_prob = symbol_prob > 1e-20? symbol_prob : 1e-20; // to avoid log(0)

  theoretical_bits  += -log2(symbol_prob);

}


#endif // CORE_ANALYSIS_UTILS_H
