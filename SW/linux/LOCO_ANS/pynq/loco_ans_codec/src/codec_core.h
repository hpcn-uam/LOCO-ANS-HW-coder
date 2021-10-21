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

#ifndef CODEC_CORE_H
#define CODEC_CORE_H

// #define NDEBUG
#include <assert.h>

#include <opencv2/imgproc/imgproc.hpp>

#include <stdint.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional> 
#include <cmath>
#include <cstring> //memcopy

#include "coder_config.h"

constexpr int MAX_IBPP = 8;
#define ERROR_REDUCTION true
#define Orig_CTX_Quant false
#define MU_estim_like_original false
#define MAX_SUPPORTED_BPP (32) // has to be mult of 8

// SYMBOL_ENDIANNESS_LITTLE:
// packs of bits stored in little endian
// new packs are assigned to less significant bits 
// SYMBOL_ENDIANNESS_LITTLE has precedence over BIT_ENDIANNESS_LITTLE
#define SYMBOL_ENDIANNESS_LITTLE true 

// BIT_ENDIANNESS_LITTLE:
// packs of bits stored in big endian
// new packs are assigned to less significant bits 
#define BIT_ENDIANNESS_LITTLE false

// #define DEBUG_ENC false
// #define DEBUG_DEC false

#define USING_DIV_RED_LUT 1

#define CHROMA_MODE_GRAY 0
#define CHROMA_MODE_YUV420 1
#define CHROMA_MODE_YUV422 2
#define CHROMA_MODE_YUV444 3
#define CHROMA_MODE_BAYER 4

#define ITERATIVE_ST true
// #define ITERATIVE_ST false


#define DBG_INFO "FILE: "<<__FILE__<<". FUNC: "<<__func__<<". LINE: "<<__LINE__<<". "
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

#define likely(x) __builtin_expect ((x), 1)
#define unlikely(x) __builtin_expect ((x), 0)

#define ENCODER_MODE_ENCODE 0
#define ENCODER_MODE_SYSTEM_TEST 1

#define ENCODER_PRED_LOCO 0


uint32_t encode_core(const cv::Mat& src,
                          cv::Mat & quant_img, 
                          uint8_t* binary_file,
                          char chroma_mode=CHROMA_MODE_YUV444,
                          char _fixed_prediction_alg = ENCODER_PRED_LOCO, // not currently in use
                          int near = 1, 
                          char encoder_mode=0, 
                          int ibpp=8);

void decode_core(unsigned char* in_file ,cv::Mat& decode_img,
                        char chroma_mode=CHROMA_MODE_YUV444, 
                        char _fixed_prediction_alg = ENCODER_PRED_LOCO, // not currently in use
                        int near = 1,  
                        uint ee_buffer_size = 2096,
                        int ibpp=8,
                        char mode =0);

void rgb2yuv(const cv::Mat& src,cv::Mat&  dst,char chroma_mode =CHROMA_MODE_YUV444);

void yuv2rgb(const cv::Mat src, cv::Mat& dst,char chroma_mode =CHROMA_MODE_YUV444);


struct Context_t{
  int32_t id;
  int sign;
  Context_t():id(0),sign(0){}
  Context_t(int _id, int _sign):id(_id),sign(_sign){}
};


#if ADD_GRAD_4
  constexpr int extra_cols_before = 2;
#else
  constexpr int extra_cols_before = 1;
#endif

constexpr int extra_cols_after = 1;
constexpr int extra_cols= extra_cols_before + extra_cols_after;
constexpr int col_idx_off = extra_cols_before;

class RowBuffer
  {
  public:
    int cols;
    int curr_row_idx;
    unsigned int * prev_row;
    unsigned int * current_row;

    RowBuffer(int _cols):cols(_cols),curr_row_idx(0){
      prev_row = new unsigned int[_cols+extra_cols];
      current_row = new unsigned int[_cols+extra_cols];
      for(int i = 0; i < _cols+extra_cols; ++i) {
        prev_row[i]= 0;
        current_row[i]= 0;
      }
    };
    inline unsigned int retrieve(int row,int col) const{
      int row_idx= row&0x1;
      if(row_idx == curr_row_idx) {
        return current_row[col+col_idx_off];
      }else{
        return prev_row[col+col_idx_off];
      }
    }
    inline void update(unsigned int px,int col){
      current_row[col+col_idx_off] =px; 
    }
    inline void start_row(){
      /*for(unsigned i = 0; i < extra_cols_before; ++i) {
        current_row[i]= prev_row[extra_cols_before];
      }*/
    }

    inline void end_row(){
      // copy last sample to extra col on the right 
      current_row[cols+col_idx_off] =current_row[cols-1+col_idx_off]; 

      // swap row pointers
      unsigned int * aux = prev_row;
      prev_row = current_row;
      current_row = aux;
      curr_row_idx = curr_row_idx?1:0;

      for(unsigned i = 0; i < extra_cols_before; ++i) {
        current_row[i]= prev_row[extra_cols_before];
      }

    }
    inline void get_teplate(int col,
                    int &a,int &b,int &c,int &d) const{
      a = current_row[col+col_idx_off-1];
      b = prev_row[col+col_idx_off];
      c = prev_row[col+col_idx_off-1];
      d = prev_row[col+col_idx_off+1];
    }

    inline void get_teplate(int col,
                    int &a,int &b,int &c,int &d,int &e) const{
      get_teplate( col, a, b, c, d);
      e = current_row[col+col_idx_off-2];
    }

    ~RowBuffer(){
      delete [] prev_row;
      delete [] current_row;
    };
    
  };



struct ee_symb_data {
  ee_symb_data():z(0),y(0),remainder_reduct_bits(0),theta_id(0),p_id(0){}
  uint16_t z;
  uint8_t y;
  uint8_t remainder_reduct_bits; //reminder_bits =  EE_REMAINDER_SIZE - remainder_reduct_bits
  uint16_t theta_id;
  uint16_t p_id;

  // int32_t z;
  // int32_t theta_id;
  // int32_t p_id;
  // int32_t y;
  // int32_t remainder_reduct_bits; //reminder_bits =  EE_REMAINDER_SIZE - remainder_reduct_bits


}__attribute__((packed));


#endif /* CODEC_CORE_H */


