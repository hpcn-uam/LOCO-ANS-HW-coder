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

#ifndef SW_IMPLEMENTATION_HPP
#define SW_IMPLEMENTATION_HPP


#include <hls_stream.h>
#include "../../coder_config.hpp"
namespace sw_impl {

   int get_theta_idx(int ctx_cnt, int ctx_St);

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


  void image_scanner(int near,int cols, int rows,
    hls::stream<int>& src, hls::stream<ee_symb_data>& symbols);


  struct Context_t{
    int32_t id;
    int sign;
    Context_t():id(0),sign(0){}
    Context_t(int _id, int _sign):id(_id),sign(_sign){}
  };

  int gradient_quantizer(int g);

  class RowBuffer
    {
    static constexpr int extra_cols_before = 1;
    static constexpr int extra_cols_after = 1;
    static constexpr int extra_cols= extra_cols_before + extra_cols_after;
    static constexpr int col_idx_off = extra_cols_before;
    public:
      int cols;
      int curr_row_idx;
      unsigned int * prev_row;
      unsigned int * current_row;

      RowBuffer():cols(0),curr_row_idx(0){}
      RowBuffer(int _cols):cols(_cols),curr_row_idx(0){
        prev_row = new unsigned int[_cols+extra_cols];
        current_row = new unsigned int[_cols+extra_cols];
        for(int i = 0; i < _cols+extra_cols; ++i) {
          prev_row[i]= 0;
          current_row[i]= 0;
        }
      };

      void set_cols(int _cols){
        cols = _cols;
        prev_row = new unsigned int[_cols+extra_cols];
        current_row = new unsigned int[_cols+extra_cols];
        for(int i = 0; i < _cols+extra_cols; ++i) {
          prev_row[i]= 0;
          current_row[i]= 0;
        }
      }
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



  

} // sw_impl

#endif // SW_IMPLEMENTATION_HPP