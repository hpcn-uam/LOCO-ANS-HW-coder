
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include "coder.hpp"
#include "coder_config.hpp"
#include <vector>

using namespace std;
using namespace hls;
#define NUM_OF_BLCKS (4)

int main(int argc, char const *argv[])
{
  stream<coder_interf_t> in_data;
  
  int num_of_errors = 0;
  for (int blk_idx = 0; blk_idx < NUM_OF_BLCKS; ++blk_idx){
    std::vector<coder_interf_t> input_vector;
    for (int i = 0; i < BUFFER_SIZE; ++i){
      symb_data_t symb_data;
      symb_ctrl_t symb_ctrl;
      if(blk_idx == 0 && i == 0) { // first pixel
        symb_ctrl = 1;
        pixel_t px = 0xFF;
        symb_data = px;
      }else{
        symb_ctrl = 0;
        int val = i+BUFFER_SIZE*blk_idx;
        int y = val & 0x80?1:0 ;
        int z = val & 0xF;//0x7F ;
        int theta = blk_idx ;
        int p = blk_idx/2 ;
        predict_symbol_t pred_symbol(z,y,theta,p);
        symb_data = pred_symbol.to_bits();
      }
      in_data.write(bits_to_intf(symb_data ,symb_ctrl));
      input_vector.push_back(bits_to_intf(symb_data ,symb_ctrl));
    }

    stream<subsymb_t> out_data;
    coder(in_data,out_data);
    
    //invert stream as ANS acts as a LIFO
    stream<subsymb_t> inverted_subsymb;
    {
      std::vector<subsymb_t> aux_vector;
      while(!out_data.empty()) {
        subsymb_t out_symb = out_data.read();
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
      if(golden_ctrl == 1) {// first pixel 
        out_symb = inverted_subsymb.read();
        pixel_t golden_px = golden_data;
        assert(out_symb.type == SUBSYMB_BYPASS);
        assert(out_symb.end_of_block == 1);
        assert(out_symb.subsymb == golden_px);
        assert(out_symb.info == INPUT_BPP);
      }else{
        predict_symbol_t pred_golden_symbol(golden_data);
        
        // check z

        // read first symbol using 
        out_symb = inverted_subsymb.read();
        assert(out_symb.type == SUBSYMB_Z);
        assert(out_symb.info == pred_golden_symbol.theta_id);

        auto ans_symb = out_symb.subsymb;
        const auto encoder_cardinality = tANS_cardinality_table[out_symb.info];
        int module = ans_symb;
        int it = 1;

        while(ans_symb == encoder_cardinality){
          assert(out_symb.end_of_block == 0);
          it++;
          
          /*if(it >= EE_MAX_ITERATIONS) {
            module = retrive_bits(escape_bits);
            break;
          }*/

          out_symb = inverted_subsymb.read();
          ans_symb = out_symb.subsymb;
          module += ans_symb;
          assert(out_symb.info == pred_golden_symbol.theta_id);
          assert(out_symb.type == SUBSYMB_Z);
        }

        assert(module == pred_golden_symbol.z);
        if(i ==0) {
          assert(out_symb.end_of_block == 1);
        }

        // check y
        out_symb = inverted_subsymb.read();
        assert(out_symb.subsymb == pred_golden_symbol.y);
        assert(out_symb.info == pred_golden_symbol.p_id);
        assert(out_symb.type == SUBSYMB_Y);
        assert(out_symb.end_of_block == 0);

      }
      i++;

    }
  }

  return num_of_errors;
}
