
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include "ANS_coder.hpp"
#include "../../test/test.hpp"
#include "../../coder_config.hpp"
#include "../../input_buffers/src/input_buffers.hpp"
#include "../../subsym_gen/src/subsym_gen.hpp"
#include <vector>
#include <list>

using namespace std;
using namespace hls;
#define NUM_OF_BLCKS (6)

#define TEST_BUFFER_SIZE 32

#define SPLITED_FREE_KERNELS 0
  
constexpr int ANS_CODER_OUT_BYTES = OUT_WORD_BYTES;
int main(int argc, char const *argv[])
{
  stream<coder_interf_t> in_data;
  
  for (int blk_idx = 0; blk_idx < NUM_OF_BLCKS; ++blk_idx){
	std::vector<coder_interf_t> input_vector;
    cout<<"Processing block "<<blk_idx<<endl;
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
    stream<ap_uint<1>> last_block;
    input_buffers(in_data, inverted_data,last_block);

    //last_block block signal not tested in this testbench
    while (! last_block.empty()){
      auto tmp = last_block.read();
    }

    stream<subsymb_t> symbol_stream;
    while (! inverted_data.empty()){
      subsymbol_gen(inverted_data,symbol_stream);
    }

    stream<byte_block<ANS_CODER_OUT_BYTES>> byte_block_stream;
    while (! symbol_stream.empty()){
  	 ANS_coder_top(symbol_stream,byte_block_stream);
    }


    //Replicate binary stack logic
    stream<byte_block<ANS_CODER_OUT_BYTES>> inverted_byte_block;
    ap_uint<1> golden_stack_overflow;
    output_stack_sw<OUTPUT_STACK_SIZE>(byte_block_stream,inverted_byte_block,golden_stack_overflow);

    #if SYMBOL_ENDIANNESS_LITTLE
    stream<byte_block<ANS_CODER_OUT_BYTES>> packed_byte_block;
    pack_out_bytes_sw_little_endian(inverted_byte_block,packed_byte_block);
    #else
    stream<byte_block<ANS_CODER_OUT_BYTES>> &packed_byte_block=inverted_byte_block;
    // stream<byte_block<ANS_CODER_OUT_BYTES>> packed_byte_block;
    // pack_out_bytes_sw_big_endian(inverted_byte_block,packed_byte_block);
    #endif

    // Replicate AXIS to DRAM:  convert stream in array 
    unsigned mem_byte_pointer = 0;
    uint8_t* block_binary = new uint8_t[packed_byte_block.size()*ANS_CODER_OUT_BYTES+4]; 
    while(! packed_byte_block.empty()) {
      byte_block<ANS_CODER_OUT_BYTES> out_byte_block = packed_byte_block.read();
      if(packed_byte_block.empty()) {
        ASSERT(out_byte_block.is_last(),==,true," | mem_byte_pointer: "<<mem_byte_pointer);
      }else{
        ASSERT(out_byte_block.is_last(),==,false," | mem_byte_pointer: "<<mem_byte_pointer);
        ASSERT(out_byte_block.num_of_bytes(),==,ANS_CODER_OUT_BYTES," | mem_byte_pointer: "<<mem_byte_pointer);
      }
      for(unsigned j = 0; j < out_byte_block.num_of_bytes(); ++j) {
        block_binary[mem_byte_pointer] = out_byte_block.data((j+1)*8-1,j*8);
        mem_byte_pointer++;
      }

    }

    Binary_Decoder bin_decoder(block_binary,block_size);
    int i = 0;
    for (auto elem_it : input_vector){
      symb_data_t golden_data;
      symb_ctrl_t golden_ctrl;
      intf_to_bits(elem_it,golden_data,golden_ctrl);
      
      // byte_block out_byte_block;

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

      ASSERT(deco_z,== ,golden_z,"Blk: "<<blk_idx<<" | i:"<<i);
      ASSERT(deco_y,== ,golden_y,"Blk: "<<blk_idx<<" | i:"<<i);
      i++;

    }
    cout<<"  | SUCCESS"<<endl;

    ASSERT(symbol_stream.size(),==,0);
    ASSERT(byte_block_stream.size(),==,0);
    //clean up 
    delete[] block_binary;
  }

  return 0;
}

