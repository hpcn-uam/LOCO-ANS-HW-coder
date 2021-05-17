
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include "./output_stack.hpp"
#include "../../coder_config.hpp"
#include <vector>

using namespace std;
using namespace hls;
#define NUM_OF_BLCKS (8)

#define DEBUG 1

void output_stack_sw(
  stream<byte_block > &in, 
  stream<byte_block > &out){
  std::vector<byte_block> aux_vector;
  byte_block out_byte_block;
  do{
    out_byte_block = in.read();
    aux_vector.push_back(out_byte_block);
  }while(out_byte_block.last_block == 0);

  while (!aux_vector.empty()){
    byte_block out_byte_block = aux_vector.back();
    aux_vector.pop_back();
    out << out_byte_block;
  }

}

int main(int argc, char const *argv[])
{
  
  for (int blk_idx = 0; blk_idx < NUM_OF_BLCKS; ++blk_idx){
    stream<byte_block> in_hw_data,in_sw_data;
    cout<<"Processing block "<<blk_idx;
    int block_size = BUFFER_SIZE - int(blk_idx/2);
    
    //generate data
    for (int i = 0; i < block_size; ++i){
      byte_block in_elem;
      in_elem.data = i;
      in_elem.bytes = (i == block_size-1)? (i%OUT_WORD_BYTES)+1 : OUT_WORD_BYTES ;
      in_elem.last_block = (i == block_size-1)?1:0 ;
      in_hw_data << in_elem;
      in_sw_data << in_elem;
    }

    stream<byte_block> out_hw_data,out_sw_data;
    output_stack(in_hw_data,out_hw_data);
    output_stack_sw(in_sw_data,out_sw_data);
    
    ASSERT(out_hw_data.size(),==,out_sw_data.size(),"Blk "<<blk_idx);

    int i = 0;
    while(!out_hw_data.empty()) {
      byte_block out_hw_elem = out_hw_data.read();
      byte_block out_sw_elem = out_sw_data.read();
      ASSERT(out_hw_elem.data,==,out_sw_elem.data,"Blk "<<blk_idx <<" | i:"<<i)
      ASSERT(out_hw_elem.bytes,==,out_sw_elem.bytes,"Blk "<<blk_idx <<" | i:"<<i)
      ASSERT(out_hw_elem.last_block,==,out_sw_elem.last_block,"Blk "<<blk_idx <<" | i:"<<i)

      i++;
    }

    cout<<"| SUCCESS"<<endl;
  }

  return 0;
}
