
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include "TSG_coder.hpp"
#include "../modules/test/test.hpp"
#include <vector>
#include <list>

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
    cout<<"Processing block "<<blk_idx<<endl;

    // ************
    // Generate input 
    // ************
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

    // ************
    // Run DUT :
    // ************
    stream<byte_block> byte_block_stream;
    TSG_coder(in_data,byte_block_stream);

    // ************
    // Check output
    // ************
    
    //Replicate binary stack logic
    stream<byte_block> inverted_byte_block;
    {
      std::vector<byte_block> aux_vector;
      while(!byte_block_stream.empty()) {
        byte_block out_byte_block = byte_block_stream.read();
        aux_vector.push_back(out_byte_block);
      }
      while (!aux_vector.empty()){
        byte_block out_byte_block = aux_vector.back();
        aux_vector.pop_back();
        inverted_byte_block << out_byte_block;
      }

    }

    // Replicate AXIS to DRAM:  convert stream in array 
    unsigned num_of_out_words = inverted_byte_block.size();
    uint32_t* block_binary = new uint32_t[num_of_out_words*OUT_WORD_BYTES]; 
    uint block_binary_ptr = 0;
    while(! inverted_byte_block.empty()) {
      byte_block out_byte_block = inverted_byte_block.read();
      if(block_binary_ptr==0) {
        ASSERT(out_byte_block.last_block,==,1,"Blk: "<<blk_idx<<" | block_binary_ptr:"<<block_binary_ptr);
      }else{
        ASSERT(out_byte_block.last_block,==,0,"Blk: "<<blk_idx<<" | block_binary_ptr:"<<block_binary_ptr);
        ASSERT(out_byte_block.bytes,==,OUT_WORD_BYTES,"Blk: "<<blk_idx<<" | block_binary_ptr:"<<block_binary_ptr);
      }
      block_binary[block_binary_ptr] = out_byte_block.data;
      block_binary_ptr++;
      // while(out_byte_block.bytes >0) {
      //   uint mask = ((1<<sizeof(block_binary)*8)-1);
      //   block_binary[block_binary_ptr]= (out_byte_block.data>>) & mask;
      //   out_byte_block.bytes-= sizeof(block_binary);
      //   out_byte_block.data >>=sizeof(block_binary)*8;
      // }

    }

    // Decode and check
    Binary_Decoder bin_decoder(block_binary,block_size);
    int i = 0;
    for (auto elem_it : input_vector){
      // get golden values
      symb_data_t golden_data;
      symb_ctrl_t golden_ctrl;
      intf_to_bits(elem_it,golden_data,golden_ctrl);

      ap_uint<Z_SIZE> golden_z ;
      ap_uint<Y_SIZE> golden_y ;
      ap_uint<THETA_SIZE> golden_theta_id ;
      ap_uint<P_SIZE> golden_p_id ;
      (golden_z,golden_y,golden_theta_id,golden_p_id) = golden_data;
      
      //decode 
      const int remainder_bits = EE_REMAINDER_SIZE - 0;
      int deco_z, deco_y;
      bin_decoder.retrive_TSG_symbol(golden_theta_id, 
                  golden_p_id, remainder_bits, deco_z, deco_y);

      //check
      ASSERT(deco_z,== ,golden_z,"Blk: "<<blk_idx<<" | i:"<<i);
      ASSERT(deco_y,== ,golden_y,"Blk: "<<blk_idx<<" | i:"<<i);
      i++;

    }
    cout<<"  | SUCCESS"<<endl;

    //clean up 
    delete[] block_binary;
  }

  return num_of_errors;
}
