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
#include "subsym_gen.hpp"
#include "../../coder_config.hpp"
#include "../../input_buffers/src/input_buffers.hpp"
#include <vector>

using namespace std;
using namespace hls;
#define NUM_OF_BLCKS (4)

#define TEST_BUFFER_SIZE 32
int main(int argc, char const *argv[])
{
  stream<coder_interf_t> in_data;
  

  int num_of_errors = 0;
  for (int blk_idx = 0; blk_idx < NUM_OF_BLCKS; ++blk_idx){
	std::vector<coder_interf_t> input_vector;
    cout<<"Processing block "<<blk_idx;
    int block_size = TEST_BUFFER_SIZE - int(blk_idx/2);
    for (int i = 0; i < block_size; ++i){
      symb_data_t symb_data;
      ap_uint<1> last_symbol = (i == block_size-1)? 1:0 ;
      ap_uint<REM_REDUCT_SIZE> in_remainder_reduct = blk_idx <=7? blk_idx: 7;

      int val = i+TEST_BUFFER_SIZE*blk_idx;
      ap_uint<Z_SIZE> z = blk_idx <= 1? val & 0xF : val & 0x7F ;
      ap_uint<Y_SIZE> y = val & 0x80?1:0 ; 
      ap_uint<THETA_SIZE> theta_id = i >= NUM_ANS_THETA_MODES?NUM_ANS_THETA_MODES-1: i ;
      ap_uint<P_SIZE> p_id = blk_idx/2 ;
      symb_data = (z,y,theta_id,p_id);
      coder_interf_t in_inf_data = (last_symbol,in_remainder_reduct,symb_data);
      in_data <<in_inf_data;
      // in_data.write(bits_to_intf(symb_data ,symb_ctrl));
      input_vector.push_back(in_inf_data);
    }

    stream<coder_interf_t> inverted_data;
    stream<ap_uint<1>> last_block;
    input_buffers(in_data, inverted_data,last_block);

    //last_block block signal not tested in this testbench
    while (! last_block.empty()){
      auto tmp = last_block.read();
    }

    stream<subsymb_t> symbol_stream;
    #ifdef SUBSYMBOL_GEN_DOUBLE_LANE_TOP
      //duplicate
      stream<coder_interf_t> inverted_data_0,inverted_data_1;
      while (! inverted_data.empty()){
        auto d = inverted_data.read();
        inverted_data_0 << d;
        inverted_data_1 << d;
      }

      stream<subsymb_t> symbol_stream_0,symbol_stream_1;
      //run DUT
      while(! inverted_data_0.empty()) {
        subsymbol_gen_double_lane(inverted_data_0,symbol_stream_0,
            inverted_data_1,symbol_stream_1);
      }
      ASSERT(inverted_data_1.size(),==,0);
      ASSERT(symbol_stream_0.size(),==,symbol_stream_1.size());

      //de-duplicate
      while (! symbol_stream_0.empty()){
        auto out_0 = symbol_stream_0.read();
        auto out_1 = symbol_stream_1.read();
        ASSERT(out_0.subsymb,==,out_1.subsymb);
        ASSERT(out_0.type,==,out_1.type);
        ASSERT(out_0.info,==,out_1.info);
        ASSERT(out_0.end_of_block,==,out_1.end_of_block);

        symbol_stream << out_0;
      }

    #else
      while (! inverted_data.empty()){
          subsymbol_gen(inverted_data,symbol_stream);
      }
    #endif

    // while(!symbol_stream.empty()) {
    //     subsymb_t aux = symbol_stream.read();
    // }

    //invert stream as ANS acts as a LIFO
    stream<subsymb_t> inverted_subsymb;
    {
      std::vector<subsymb_t> aux_vector;
      while(!symbol_stream.empty()) {
        subsymb_t out_symb = symbol_stream.read();
        cout<< " |type: "<<out_symb.type;
	  cout<< " |info: "<<out_symb.info;
	  cout<< " |subsymb: "<<out_symb.subsymb;
	  cout<< " |end_of_block: "<<out_symb.end_of_block<<endl;
        aux_vector.push_back(out_symb);
      }
      while (!aux_vector.empty()){
        subsymb_t out_symb = aux_vector.back();
        aux_vector.pop_back();
        inverted_subsymb << out_symb;
      }

    }

    int i = 0;

    for (auto elem_it = input_vector.begin(); elem_it != input_vector.end(); ++elem_it){
      symb_data_t golden_data;
      symb_ctrl_t golden_ctrl;
      
      intf_to_bits(*elem_it,golden_data,golden_ctrl);
      
      subsymb_t out_symb;

      ap_uint<Z_SIZE> golden_z ;
      ap_uint<Y_SIZE> golden_y ;
      ap_uint<THETA_SIZE> golden_theta_id ;
      ap_uint<P_SIZE> golden_p_id ;
      (golden_z,golden_y,golden_theta_id,golden_p_id) = golden_data;

      ap_uint<1> old_last_symbol ;
      ap_uint<REM_REDUCT_SIZE> golden_remainder_reduct;
      (old_last_symbol,golden_remainder_reduct)=golden_ctrl;
      
      // check z

      // read first symbol using 
      out_symb = inverted_subsymb.read();

      cout<< " |type: "<<out_symb.type;
      cout<< " |info: "<<out_symb.info;
      cout<< " |subsymb: "<<out_symb.subsymb;
      cout<< " |end_of_block: "<<out_symb.end_of_block<<endl;

      ASSERT(out_symb.type ,== ,SUBSYMB_Z_LAST,"i: "<<i);
      ASSERT(out_symb.info ,== , golden_theta_id,"i: "<<i);

      auto ans_symb = out_symb.subsymb;
      const auto encoder_cardinality = tANS_cardinality_table[out_symb.info];
      int module = ans_symb;
      int it = 1;

      if(i ==0) {
		    ASSERT(out_symb.end_of_block ,== , 1,"i: "<<i);
  		}else{
  			ASSERT(out_symb.end_of_block ,== , 0,"i: "<<i);
  		}

      while(ans_symb == encoder_cardinality){
        out_symb = inverted_subsymb.read();

        if(it >= EE_MAX_ITERATIONS) {
          const int remainder_bits = EE_REMAINDER_SIZE - golden_remainder_reduct;
          ASSERT(out_symb.type ,== , SUBSYMB_BYPASS,"i: "<<i);
          ASSERT(out_symb.info ,== , remainder_bits,"i: "<<i);
          ASSERT(out_symb.end_of_block ,== , 0,"i: "<<i);
          module = out_symb.subsymb;
          break;
        }


        ans_symb = out_symb.subsymb;
        module += ans_symb;
        ASSERT(out_symb.info ,== , golden_theta_id,"i: "<<i);
        ASSERT(out_symb.type ,== , SUBSYMB_Z ,"i: "<<i);
        ASSERT(out_symb.end_of_block ,== , 0,"i: "<<i);
        
        it++;
      }

      ASSERT(module ,== , golden_z,"i: "<<i);


      // check y
      out_symb = inverted_subsymb.read();
      ASSERT(out_symb.subsymb ,== , golden_y,"i: "<<i);
      ASSERT(out_symb.info ,== , golden_p_id,"i: "<<i);
      ASSERT(out_symb.type ,== , SUBSYMB_Y,"i: "<<i);
      ASSERT(out_symb.end_of_block ,== , 0,"i: "<<i);

      i++;

    }
    ASSERT(inverted_data.size(),==,0);
    ASSERT(symbol_stream.size(),==,0);
    cout<<"| SUCCESS"<<endl;
  }

  return num_of_errors;
}
