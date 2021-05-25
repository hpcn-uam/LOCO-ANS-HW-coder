
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
#define NUM_OF_BLCKS (8)

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
      ap_uint<Z_SIZE> z = blk_idx <= 3? val&0x1:(blk_idx <= 5? val & 0xF : val & 0x7F) ;
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
    // stream<byte_block> byte_block_stream;
    stream<byte_block<OUT_DMA_BYTES>> packed_byte_block;
    TSG_coder(in_data,packed_byte_block);
    // stream<byte_block<OUT_WORD_BYTES>> inverted_byte_block;
    // TSG_coder(in_data,inverted_byte_block);


    // ************
    // Check output
    // ************
    

    //pack bytes
    /*#if SYMBOL_ENDIANNESS_LITTLE
    pack_out_bytes_sw_little_endian(inverted_byte_block,packed_byte_block);
    #else
    stream<byte_block<OUT_WORD_BYTES>> &packed_byte_block=inverted_byte_block;
    #endif*/

    // Replicate AXIS to DRAM:  convert stream in array 
    unsigned mem_byte_pointer = 0;
    uint8_t* block_binary = new uint8_t[packed_byte_block.size()*OUT_DMA_BYTES+4]; 
    while(! packed_byte_block.empty()) {
      byte_block<OUT_DMA_BYTES> out_byte_block = packed_byte_block.read();
      if(packed_byte_block.empty()) {
        ASSERT(out_byte_block.is_last(),==,true," | mem_byte_pointer: "<<mem_byte_pointer);
      }else{
        ASSERT(out_byte_block.is_last(),==,false," | mem_byte_pointer: "<<mem_byte_pointer);
        ASSERT(out_byte_block.num_of_bytes(),==,OUT_DMA_BYTES," | mem_byte_pointer: "<<mem_byte_pointer);
      }
      for(unsigned j = 0; j < out_byte_block.num_of_bytes(); ++j) {
        block_binary[mem_byte_pointer] = out_byte_block.data((j+1)*8-1,j*8);
        mem_byte_pointer++;
      }

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
