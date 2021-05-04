
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
  
  std::vector<coder_interf_t> input_vector;
  int num_of_errors = 0;
  for (int blk_idx = 0; blk_idx < NUM_OF_BLCKS; ++blk_idx){
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
    
    // for (int i = BUFFER_SIZE-1; i>=0; --i){

    int i = BUFFER_SIZE-1;
    // Verify output
    subsymb_t out_symb;  
    bool need_to_read = true;
    while(!input_vector.empty()){
      symb_data_t golden_data;
      symb_ctrl_t golden_ctrl;
      intf_to_bits(input_vector.back(),golden_data,golden_ctrl);
      input_vector.pop_back();

      // intf_to_bits(out_data.read(),symb_data,symb_ctrl);
      
      if(golden_ctrl == 1) {// first pixel 
        out_symb = need_to_read?out_data.read():out_symb;
        pixel_t golden_px = golden_data;
        assert(out_symb.subsymb == golden_px);
        assert(out_symb.type == SUBSYMB_BYPASS);
        assert(out_symb.end_of_block == 1);
        assert(out_symb.info == INPUT_BPP);
        need_to_read = true;
      }else{
        predict_symbol_t pred_golden_symbol(golden_data);
        // check y
        out_symb = need_to_read?out_data.read():out_symb;
        need_to_read = true;
        assert(out_symb.subsymb == pred_golden_symbol.y);
        assert(out_symb.info == pred_golden_symbol.p_id);
        assert(out_symb.type == SUBSYMB_Y);
        assert(out_symb.end_of_block == 0);
        
        // check z

        // read first symbol using 
        out_symb = out_data.read();
        assert(out_symb.type == SUBSYMB_Z);
        assert(out_symb.info == pred_golden_symbol.theta_id);

        auto ans_symb = out_symb.subsymb;
        const auto encoder_cardinality = tANS_cardinality_table[out_symb.info];
        int module = 0;
        int it = 1;


        subsymb_t new_out_symb=out_symb; 
        do{
          out_symb =new_out_symb;
          ans_symb = out_symb.subsymb;
          // assert(out_symb.end_of_block == 0);
          assert(out_symb.info == pred_golden_symbol.theta_id);
          assert(out_symb.type == SUBSYMB_Z);
          module += ans_symb;
          
          /*if(it >= EE_MAX_ITERATIONS) {
            module = retrive_bits(escape_bits);
            break;
          }*/

          new_out_symb = out_data.read();
          it++;
        }while(new_out_symb.type == SUBSYMB_Z);

        need_to_read = false;
        if(module != pred_golden_symbol.z) {
          cout<<"symbol "<<i<<": | Module: "<<module<<" instead of "<<
          pred_golden_symbol.z<<endl;
        }
        assert(module == pred_golden_symbol.z);
        if(input_vector.empty()) {
          assert(out_symb.end_of_block == 1);
        }

        out_symb = new_out_symb;

      }

      i--;
    }
  }

  return num_of_errors;
}
