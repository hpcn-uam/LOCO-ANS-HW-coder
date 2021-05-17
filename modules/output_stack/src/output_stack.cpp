
#include "output_stack.hpp"


void write_binary_stack(
  stream<byte_block> &in, 
  out_word_t buff[OUTPUT_STACK_SIZE],
  ap_uint<OUTPUT_STACK_ADDRESS_SIZE> &last_element,
  decltype(byte_block::bytes) &last_elem_bytes,
  ap_uint<1> &stack_overflow){

  BUILD_BUG_ON(floorlog2(OUTPUT_STACK_SIZE)!= ceillog2(OUTPUT_STACK_SIZE));
  // ASSERT(ceil(log2(OUTPUT_STACK_SIZE)),==, log2(OUTPUT_STACK_SIZE), 
  //   "Coder_config: OUTPUT_STACK_SIZE has to be 2^int"); 

  /*to bit stack logic
        ASSERT(stack_ptr,>=,0,"ERROR: Stack overflow. MAX_SUPPORTED_BPP ("<<
          MAX_SUPPORTED_BPP<<") it's not enough." );*/
  byte_block new_element;
  stack_overflow = 0;
  ap_uint<OUTPUT_STACK_ADDRESS_SIZE +1> elem_ptr = 0;
  write_binary_stack_loop: do{
  #pragma HLS PIPELINE 
    in >> new_element;
    if (new_element.last_block == 1){
      last_element = elem_ptr;
      last_elem_bytes = new_element.bytes;
    }

    buff[elem_ptr(decltype(elem_ptr)::width-2,0)] = new_element.data;

    // ASSERT(OUTPUT_STACK_SIZE,>,elem_ptr.to_int()+1," Output stack overflow");
    // ASSERT(decltype(elem_ptr)(elem_ptr+1),>,elem_ptr," Output stack pointer overflow");

    if(elem_ptr[decltype(elem_ptr)::width-1] == 1 ) {
      stack_overflow = 1;
    }
    elem_ptr++;
  }while(new_element.last_block == 0);

}

void read_binary_stack(
  const out_word_t buff[OUTPUT_STACK_SIZE],
  ap_uint<OUTPUT_STACK_ADDRESS_SIZE> last_element, 
  decltype(byte_block::bytes) last_elem_bytes,
  stream<byte_block> &out  
  ){


  decltype(byte_block::bytes) elem_bytes = last_elem_bytes;
  read_binary_stack_loop:for (int elem_ptr = last_element; elem_ptr >=0 ; --elem_ptr){
    byte_block new_element;
    #pragma HLS LOOP_TRIPCOUNT max=OUTPUT_STACK_SIZE
    #pragma HLS PIPELINE 
    new_element.data = buff[elem_ptr];
    new_element.last_block = (elem_ptr == 0)? 1 : 0; 
    new_element.bytes = elem_bytes;
    elem_bytes = OUT_WORD_BYTES; // following are full

    //TODO perform byte aligning
    out << new_element;
  }
}


void output_stack(
  stream<byte_block > &in, 
  stream<byte_block > &out,
  ap_uint<1> &stack_overflow){
  #if OUTPUT_STACK_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=in
  #pragma HLS INTERFACE axis register_mode=both register port=out
  #endif

  #pragma HLS DATAFLOW
  #pragma HLS INTERFACE ap_ctrl_none port=return

  out_word_t binary_stack[OUTPUT_STACK_SIZE];
  #pragma HLS STREAM variable=binary_stack depth=2 off
  ap_uint<OUTPUT_STACK_ADDRESS_SIZE> last_element;
  decltype(byte_block::bytes) last_elem_bytes;
  // ap_uint<LOG2_OUT_WORD_BYTES+1> last_elem_bytes;

  write_binary_stack(in,binary_stack,last_element,last_elem_bytes,stack_overflow);
  read_binary_stack(binary_stack, last_element,last_elem_bytes,out);
}


