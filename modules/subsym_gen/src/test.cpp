
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
      symb_ctrl_t symb_ctrl = (i == block_size-1)? 1:0 ;

      int val = i+TEST_BUFFER_SIZE*blk_idx;
      ap_uint<Z_SIZE> z = blk_idx <= 1? val & 0xF : val & 0x7F ;
      ap_uint<Y_SIZE> y = val & 0x80?1:0 ; 
      ap_uint<THETA_SIZE> theta_id = i >= NUM_ANS_THETA_MODES?NUM_ANS_THETA_MODES-1: i ;
      ap_uint<P_SIZE> p_id = blk_idx/2 ;
      symb_data = (z,y,theta_id,p_id);
      in_data.write(bits_to_intf(symb_data ,symb_ctrl));
      input_vector.push_back(bits_to_intf(symb_data ,symb_ctrl));
    }

    stream<coder_interf_t> inverted_data;
    input_buffers(in_data, inverted_data);

    stream<subsymb_t> symbol_stream;
    while (! inverted_data.empty()){
        sub_symbol_gen(inverted_data,symbol_stream);
    }

    // while(!symbol_stream.empty()) {
    //     subsymb_t aux = symbol_stream.read();
    // }

    //invert stream as ANS acts as a LIFO
    stream<subsymb_t> inverted_subsymb;
    {
      std::vector<subsymb_t> aux_vector;
      while(!symbol_stream.empty()) {
        subsymb_t out_symb = symbol_stream.read();
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
      
      // check z

      // read first symbol using 
      out_symb = inverted_subsymb.read();

      assert(out_symb.type ==SUBSYMB_Z_LAST);
      assert(out_symb.info == golden_theta_id);

      auto ans_symb = out_symb.subsymb;
      const auto encoder_cardinality = tANS_cardinality_table[out_symb.info];
      int module = ans_symb;
      int it = 1;

      if(i ==0) {
		  assert(out_symb.end_of_block == 1);
		}else{
			assert(out_symb.end_of_block == 0);
		}

      while(ans_symb == encoder_cardinality){

        it++;
        
        /*if(it >= EE_MAX_ITERATIONS) {
          module = retrive_bits(escape_bits);
          break;
        }*/

        out_symb = inverted_subsymb.read();

        ans_symb = out_symb.subsymb;
        module += ans_symb;
        assert(out_symb.info == golden_theta_id);
        assert(out_symb.type == SUBSYMB_Z );
        assert(out_symb.end_of_block == 0);
      }

      assert(module == golden_z);


      // check y
      out_symb = inverted_subsymb.read();
      assert(out_symb.subsymb == golden_y);
      assert(out_symb.info == golden_p_id);
      assert(out_symb.type == SUBSYMB_Y);
      assert(out_symb.end_of_block == 0);

      i++;

    }

    cout<<"| SUCCESS"<<endl;
  }

  return num_of_errors;
}
