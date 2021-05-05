
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

#define DEBUG 1
int main(int argc, char const *argv[])
{
  stream<coder_interf_t> in_data;
  int num_of_errors = 0;
  std::vector<coder_interf_t> input_vector;
  
  for (int blk_idx = 0; blk_idx < NUM_OF_BLCKS; ++blk_idx){
    cout<<"Processing block "<<blk_idx;
    int block_size = BUFFER_SIZE - int(blk_idx/2);
    for (int i = 0; i < block_size; ++i){
      symb_data_t symb_data;
      symb_ctrl_t symb_ctrl = (i == block_size-1)? 1:0 ;

      int val = i+BUFFER_SIZE*blk_idx;
      ap_uint<Z_SIZE> z = val & 0x80?1:0 ;
      ap_uint<Y_SIZE> y = val & 0xF;//0x7F ;
      ap_uint<THETA_SIZE> theta_id = blk_idx ;
      ap_uint<P_SIZE> p_id = blk_idx/2 ;
      symb_data = (z,y,theta_id,p_id);
      in_data.write(bits_to_intf(symb_data ,symb_ctrl));
      input_vector.push_back(bits_to_intf(symb_data ,symb_ctrl));
    }

    stream<coder_interf_t> out_data;
    input_buffers(in_data,out_data);
    
    // for (int i = BUFFER_SIZE-1; i>=0; --i){

    // Verify output
    while(!input_vector.empty()){
      symb_data_t golden_data;
      symb_ctrl_t old_ctrl;
      intf_to_bits(input_vector.back(),golden_data,old_ctrl);
      input_vector.pop_back();
      symb_ctrl_t golden_ctrl = input_vector.empty()? 1: 0;

      symb_data_t symb_data;
      ap_uint<1> last_symbol;
      intf_to_bits(out_data.read(),symb_data,last_symbol);

      #if DEBUG
        int db_golden_data = golden_data;
        int db_golden_ctrl = golden_ctrl;
        int db_symb_data = symb_data;
        int db_last_symbol = last_symbol;
      #endif

      assert(symb_data == golden_data);
      assert(last_symbol == golden_ctrl);     
    }
    cout<<"| SUCCESS"<<endl;
  }

  return num_of_errors;
}
