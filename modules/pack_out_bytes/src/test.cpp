
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include "./pack_out_bytes.hpp"
#include "../../coder_config.hpp"
#include <vector>

using namespace std;
using namespace hls;
#define NUM_OF_BLCKS (8)


void pack_out_bytes_sw(
  stream<byte_block> &in_bytes,
  stream<byte_block> &out_bitstream){

  //state variables
  static uint byte_ptr=0;
  static long long unsigned byte_buffer=0;

  while(!in_bytes.empty()) {
    byte_block in_block;
    in_bytes >> in_block;

    in_block.data &= decltype(in_block.data)((8<<in_block.bytes)-1); // ensure upper bits are zero

    byte_buffer |= decltype(byte_buffer)(in_block.data) << (byte_ptr*8); 

    byte_ptr += in_block.bytes;
    ASSERT(byte_ptr,<=,sizeof(byte_buffer)); // check no overflow

    byte_block out_byte_block;
    if(byte_ptr >= OUT_WORD_BYTES) {
      out_byte_block.data = out_word_t(byte_buffer);// select lower bits
      out_byte_block.bytes = OUT_WORD_BYTES;
      out_byte_block.last_block = byte_ptr == OUT_WORD_BYTES? in_block.last_block : ap_uint<1> (0);

      out_bitstream << out_byte_block; 
      byte_ptr -= OUT_WORD_BYTES; // OPT: OUT_WORD_BYTES is a power of 2, then I can just truncate
      byte_buffer >>=OUT_WORD_BYTES;
    }

    ASSERT(byte_ptr,<,OUT_WORD_BYTES ); 
    if(in_block.last_block == 1 && byte_ptr >0){ // send the data in the byte_buffer
      byte_block out_byte_block;
      out_byte_block.data = out_word_t(byte_buffer);// select lower bits
      out_byte_block.bytes = byte_ptr;
      out_byte_block.last_block = 1;
      out_bitstream << out_byte_block; 

      // reset
      byte_ptr = 0;
      byte_buffer=0;
    }

    if(in_block.last_block == 1) {
      ASSERT(byte_ptr,==,0)
      ASSERT(byte_buffer,==,0)
    }
  }

}


int main(int argc, char const *argv[])
{
  
  for (int blk_idx = 0; blk_idx < NUM_OF_BLCKS; ++blk_idx){
    stream<byte_block> in_hw_data,in_sw_data;
    cout<<"Processing block "<<blk_idx;
    int block_size = OUTPUT_STACK_SIZE - int(blk_idx/2);
    
    //generate data
    for (int i = 0; i < block_size; ++i){
      byte_block in_elem;
      in_elem.data = i;
      in_elem.bytes = (i%(OUT_WORD_BYTES+1)) ;
      in_elem.last_block = (i == block_size-1)?1:0 ;
      in_hw_data << in_elem;
      in_sw_data << in_elem;
    }

    stream<byte_block> out_hw_data,out_sw_data;
    pack_out_bytes(in_hw_data,out_hw_data);
    pack_out_bytes_sw(in_sw_data,out_sw_data);
    
    ASSERT(out_hw_data.size(),==,out_sw_data.size(),"Blk "<<blk_idx);

    int i = 0;
    while(!out_hw_data.empty()) {
      byte_block out_hw_elem = out_hw_data.read();
      byte_block out_sw_elem = out_sw_data.read();
      ASSERT(out_hw_elem.data,==,out_sw_elem.data,"Blk "<<blk_idx <<" | i:"<<i)
      ASSERT(out_hw_elem.bytes,==,out_sw_elem.bytes,"Blk "<<blk_idx <<" | i:"<<i)
      ASSERT(out_hw_elem.last_block,==,out_sw_elem.last_block,"Blk "<<blk_idx <<" | i:"<<i)
      if(!out_hw_data.empty()) {
        ASSERT(out_hw_elem.bytes,==,OUT_WORD_BYTES,"Blk "<<blk_idx <<" | i:"<<i)
        ASSERT(out_hw_elem.last_block,==,0,"Blk "<<blk_idx <<" | i:"<<i)
      }else{
        ASSERT(out_hw_elem.last_block,==,1,"Blk "<<blk_idx <<" | i:"<<i)
      }

      i++;
    }

    cout<<"| SUCCESS"<<endl;
  }

  return 0;
}
