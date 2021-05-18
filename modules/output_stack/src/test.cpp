
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

// #define DEBUG 1

void output_stack_sw(
  stream<byte_block > &in, 
  stream<byte_block > &out,
  ap_uint<1> &golden_stack_overflow){
  std::vector<byte_block> aux_vector;
  byte_block in_byte_block;
  bool first_read = true;
  do{
    in_byte_block = in.read();
    auto out_byte_block = in_byte_block;
    out_byte_block.last_block = first_read? 1:0;
    aux_vector.push_back(out_byte_block);
    first_read = false;
  }while(in_byte_block.last_block == 0);

  while (!aux_vector.empty()){
    byte_block out_byte_block = aux_vector.back();
    aux_vector.pop_back();
    out << out_byte_block;
  }

  golden_stack_overflow = out.size()> OUTPUT_STACK_SIZE? 1:0;

}

int main(int argc, char const *argv[])
{
  bool is_cosim = false;
  if(argc>1 && atoi(argv[1])==1) {
    is_cosim=true;
    cout<< "Running RTL Cosim"<<endl;
  }else{
    cout<< "Running Csim"<<endl;
  }

  for (int blk_idx = 0; blk_idx < NUM_OF_BLCKS; ++blk_idx){
    stream<byte_block> in_hw_data,in_sw_data;
    cout<<"Processing block "<<blk_idx;
    int block_size = OUTPUT_STACK_SIZE +10 - int(blk_idx/2)*5;
    
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
    ap_uint<1> stack_overflow, golden_stack_overflow;
    output_stack(in_hw_data,out_hw_data,stack_overflow);
    output_stack_sw(in_sw_data,out_sw_data,golden_stack_overflow);

    if(is_cosim) {  
      // stack_overflow is a static, ap_vld port. Cosim cannot verify it
      cout<<"| Warning: Cosim cannot verify overflow flag (inspect waves to check it)";
    }else{
      ASSERT(stack_overflow, == , golden_stack_overflow,"Blk "<<blk_idx);
    }

    if(golden_stack_overflow ==1) {
      while(!out_hw_data.empty()) {
        byte_block out_hw_elem = out_hw_data.read();
      }
      while(!out_sw_data.empty()) {
        byte_block out_sw_elem = out_sw_data.read();
      }

      cout<<"| SUCCESS (Overflow correctly detected, data ignored)"<<endl;
      continue;
    }

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
