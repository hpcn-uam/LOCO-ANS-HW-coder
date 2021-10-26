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

#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include "./input_buffers.hpp"
#include "../../coder_config.hpp"
#include <vector>
#include <list>

using namespace std;
using namespace hls;
#define NUM_OF_BLCKS (6)

#define DEBUG 1
int main(int argc, char const *argv[])
{
  stream<coder_interf_t> in_data;
  int num_of_errors = 0;
  std::list<std::vector<coder_interf_t>> input_list;
  
  for (int test_idx = 0; test_idx < NUM_OF_BLCKS; ++test_idx){
    int in_stream_size = BUFFER_SIZE +10 - int(test_idx/2)*10;
    cout<<"Test "<<test_idx<<" | symbol stream size: "<<in_stream_size <<endl;

    std::vector<coder_interf_t> current_blk;
    for (int i = 0; i < in_stream_size; ++i){
      symb_data_t symb_data;
      ap_uint<1> last_symbol = (i == in_stream_size-1)? 1:0 ;
      ap_uint<REM_REDUCT_SIZE> in_remainder_reduct = test_idx <=7? test_idx: 7;

      int val = i+BUFFER_SIZE*test_idx;
      ap_uint<Z_SIZE> z = val & 0x80?1:0 ;
      ap_uint<Y_SIZE> y = val & 0xF;//0x7F ;
      ap_uint<THETA_SIZE> theta_id = test_idx ;
      ap_uint<P_SIZE> p_id = test_idx/2 ;
      symb_data = (z,y,theta_id,p_id);

      coder_interf_t in_inf_data = (last_symbol,in_remainder_reduct,symb_data);
      in_data <<in_inf_data;
      // in_data.write(bits_to_intf(symb_data ,symb_ctrl));
      current_blk.push_back(in_inf_data);
      // current_blk.push_back(bits_to_intf(symb_data ,symb_ctrl));
      if(current_blk.size() == BUFFER_SIZE) {
        input_list.push_back(current_blk);
        current_blk.clear();
      }
    }
    if(!current_blk.empty()) {
      input_list.push_back(current_blk);
      current_blk.clear();
    }


    stream<coder_interf_t> out_data;
    stream<ap_uint<1>> last_block;
    
    int blk_idx=0;
    while(!input_list.empty()){ // iterate over blocks

      // DUT 
      input_buffers(in_data,out_data,last_block);

      // Verify output
      ASSERT(last_block.size(),==,1);
      ASSERT(out_data.size(),>,0);

      cout<<"\t| Processing block "<<blk_idx<<endl;

      auto testing_in_block = input_list.front();
      input_list.pop_front();

      auto hw_last_block_elem = last_block.read();
      int golden_last_block = input_list.empty()? 1: 0;
      ASSERT(hw_last_block_elem,==,golden_last_block)

      int elem_cnt = 0;
      while(!testing_in_block.empty()){ // iterate over blocks
        symb_data_t golden_data;
        // symb_ctrl_t old_ctrl;
        ap_uint<REM_REDUCT_SIZE> golden_remainder_reduct;
        ap_uint<1> old_last_symbol;
        // intf_to_bits(testing_in_block.back(),golden_data,old_ctrl);
        (old_last_symbol, golden_remainder_reduct,golden_data) = testing_in_block.back();
        testing_in_block.pop_back();

        symb_ctrl_t golden_last_symbol = testing_in_block.empty()? 1: 0;

        symb_data_t symb_data;
        ap_uint<REM_REDUCT_SIZE> hw_remainder_reduct;
        ap_uint<1> last_symbol;
        (last_symbol,hw_remainder_reduct,symb_data) = out_data.read();
        // intf_to_bits(out_data.read(),symb_data,last_symbol);

        #if DEBUG
          int db_golden_data = golden_data;
          int db_golden_last_symbol = golden_last_symbol;
          int db_symb_data = symb_data;
          int db_last_symbol = last_symbol;
        #endif

        ASSERT(symb_data ,==, golden_data, "elem_cnt: "<<elem_cnt);
        ASSERT(last_symbol ,==, golden_last_symbol, "elem_cnt: "<<elem_cnt);     
        ASSERT(hw_remainder_reduct ,==, golden_remainder_reduct, "elem_cnt: "<<elem_cnt);
      }
      blk_idx++;
    }

    input_list.clear();
    ASSERT(input_list.size(),==,0);
    ASSERT(in_data.size(),==,0);
    ASSERT(out_data.size(),==,0);
    ASSERT(last_block.size(),==,0);

    cout<<"\t| SUCCESS"<<endl;
  }

  return num_of_errors;
}
