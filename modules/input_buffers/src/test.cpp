
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include "./input_buffers.hpp"
#include "../../coder_config.hpp"
#include <vector>

using namespace std;
using namespace hls;
#define NUM_OF_BLCKS (4)

int main(int argc, char const *argv[])
{
  stream<coder_interf_t> in_data;
  int num_of_errors = 0;
  std::vector<coder_interf_t> input_vector;
  
  for (int blk_idx = 0; blk_idx < NUM_OF_BLCKS; ++blk_idx){
    for (int i = 0; i < BUFFER_SIZE; ++i){
      symb_data_t symb_data;
      symb_ctrl_t symb_ctrl;
      if(blk_idx == 0 && i == 0) { // first pixel
        symb_ctrl = 1;
        // pixel_t px = 0xFF;
        symb_data = 0xFF;
      }else{
        symb_ctrl = 0;
        int val = i+BUFFER_SIZE*blk_idx;
        ap_uint<Z_SIZE> z = val & 0x80?1:0 ;
        ap_uint<Y_SIZE> y = val & 0xF;//0x7F ;
        ap_uint<THETA_SIZE> theta_id = blk_idx ;
        ap_uint<P_SIZE> p_id = blk_idx/2 ;
        symb_data = (z,y,theta_id,p_id);
      }
      in_data.write(bits_to_intf(symb_data ,symb_ctrl));
      input_vector.push_back(bits_to_intf(symb_data ,symb_ctrl));
    }

    stream<ap_uint<SYMB_DATA_SIZE+2>> out_data;
    input_buffers(in_data,out_data);
    
    // for (int i = BUFFER_SIZE-1; i>=0; --i){

    // Verify output
    while(!input_vector.empty()){
      symb_data_t golden_data;
      symb_ctrl_t golden_ctrl;
      intf_to_bits(input_vector.back(),golden_data,golden_ctrl);
      input_vector.pop_back();

      symb_data_t symb_data;
      ap_uint<1> last_symbol;
      ap_uint<1> is_first_px;
      (symb_data,last_symbol,is_first_px) = out_data.read();

      
      assert(symb_data == golden_data);
      assert(is_first_px == golden_ctrl);
      if(input_vector.empty()) {
        if(is_first_px ==1) {
          assert(last_symbol == 0);
        }else{
          assert(last_symbol == 1);
        }
      }else{
        symb_data_t aux_data;
        symb_ctrl_t aux_ctrl;
        intf_to_bits(input_vector.back(),aux_data,aux_ctrl);
        if(aux_ctrl ==1) { // next is first
          assert(last_symbol == 1);
        }else{
          assert(last_symbol == 0);
        }
      }
      
    }
  }

  return num_of_errors;
}
