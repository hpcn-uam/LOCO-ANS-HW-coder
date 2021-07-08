
#include "output_stack.hpp"

template<unsigned NB> // Num of bytes of input byte_blocks
void write_binary_stack(
  stream<byte_block<NB>> &in, 
  out_word_t buff[OUTPUT_STACK_SIZE],
  ap_uint<OUTPUT_STACK_ADDRESS_SIZE> &last_element,
  ap_uint<byte_block<NB>::COUNTER_WIDTH> &last_elem_bytes,
  ap_uint<1> &stack_overflow){

  BUILD_BUG_ON(floorlog2(OUTPUT_STACK_SIZE)!= ceillog2(OUTPUT_STACK_SIZE));

  byte_block<NB> in_blk;
  stack_overflow = 0;
  ap_uint<OUTPUT_STACK_ADDRESS_SIZE +1> elem_ptr = 0;
  write_binary_stack_loop: do{
    #pragma HLS LOOP_TRIPCOUNT max=OUTPUT_STACK_SIZE
    #pragma HLS PIPELINE 
    in >> in_blk;
    if (in_blk.is_last()){
      last_element = elem_ptr;
      last_elem_bytes = in_blk.num_of_bytes();
    }

    buff[elem_ptr(decltype(elem_ptr)::width-2,0)] = in_blk.data;

    // ASSERT(OUTPUT_STACK_SIZE,>,elem_ptr.to_int()+1," Output stack overflow");
    // ASSERT(decltype(elem_ptr)(elem_ptr+1),>,elem_ptr," Output stack pointer overflow");

    if(elem_ptr[decltype(elem_ptr)::width-1] == 1 && stack_overflow == 0) {
      stack_overflow = 1;
    }

    elem_ptr++;
    /*if(in_blk.num_of_bytes()!=0 || in_blk.is_last()) { // check if blk carries info
      elem_ptr++;
    }*/
  }while(!in_blk.is_last());

}

template<unsigned NB> // Num of bytes of input byte_blocks
void read_binary_stack(
  const out_word_t buff[OUTPUT_STACK_SIZE],
  ap_uint<OUTPUT_STACK_ADDRESS_SIZE> last_element, 
  ap_uint<byte_block<NB>::COUNTER_WIDTH> last_elem_bytes,
  stream<byte_block<NB>> &out,  
  stream<ap_uint<OUTPUT_STACK_BYTES_SIZE>> &last_byte_idx ){

  auto elem_bytes = last_elem_bytes;
  // Next conditional can be simplified at compile time
  if(OUT_WORD_BYTES == 1) { 
    last_byte_idx <<last_element; //straightforward for OUT_WORD_BYTES == 1
  }else{
    last_byte_idx << last_element*OUT_WORD_BYTES + last_elem_bytes -1;
  }

  read_binary_stack_loop:for (int elem_ptr = last_element; elem_ptr >=0 ; --elem_ptr){
    byte_block<NB> new_element;
    #pragma HLS LOOP_TRIPCOUNT max=OUTPUT_STACK_SIZE
    #pragma HLS PIPELINE 
    new_element.data = buff[elem_ptr];
    new_element.set_last(elem_ptr == 0); 
    new_element.set_num_of_bytes(elem_bytes);
    elem_bytes = OUT_WORD_BYTES; // following are full

    //TODO perform byte aligning
    out << new_element;
  }
}


void output_stack(
  stream<byte_block<OUT_WORD_BYTES> > &in, 
  stream<byte_block<OUT_WORD_BYTES> > &out,
  stream<ap_uint<OUTPUT_STACK_BYTES_SIZE> > &last_byte_idx,
  ap_uint<1> &stack_overflow){
  #if OUTPUT_STACK_TOP
    #pragma HLS INTERFACE axis register_mode=both register port=in
    #pragma HLS INTERFACE axis register_mode=both register port=out
    #pragma HLS INTERFACE axis register_mode=both register port=last_byte_idx
    #pragma HLS INTERFACE ap_vld register port=stack_overflow
  #endif
  #pragma HLS INTERFACE ap_ctrl_none port=return

  #pragma HLS DATAFLOW
  #pragma HLS stable variable=stack_overflow

  out_word_t binary_stack[OUTPUT_STACK_SIZE];
  #pragma HLS STREAM variable=binary_stack depth=2 off
  ap_uint<OUTPUT_STACK_ADDRESS_SIZE> last_element;
  ap_uint<byte_block<OUT_WORD_BYTES>::COUNTER_WIDTH> last_elem_bytes;

  write_binary_stack(in,binary_stack,last_element,last_elem_bytes,stack_overflow);
  read_binary_stack(binary_stack, last_element,last_elem_bytes,out,last_byte_idx);
}


