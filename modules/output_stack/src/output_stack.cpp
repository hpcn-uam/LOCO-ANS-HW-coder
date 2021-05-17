
#include "output_stack.hpp"

coder_interf_t bits_to_intf(symb_data_t symb,symb_ctrl_t ctrl){
  return (ctrl,symb);
}

void intf_to_bits(coder_interf_t intf, symb_data_t &symb,symb_ctrl_t &ctrl){
  (ctrl,symb) = intf;
}

void write_block(
  stream<coder_interf_t> &in, 
  symb_data_t buff[BUFFER_SIZE],
  ap_uint<BUFFER_ADDR_SIZE> &last_element
  ){
  // #pragma HLS INLINE

  write_block_loop:for (int elem_ptr = 0; elem_ptr < BUFFER_SIZE ; ++elem_ptr){
  // #pragma HLS PIPELINE rewind 
  #pragma HLS PIPELINE 
    symb_data_t symb_data;
    symb_ctrl_t last_symbol;
    intf_to_bits(in.read(),symb_data,last_symbol);
    // last_element = elem_ptr;
    if (elem_ptr == BUFFER_SIZE-1 || last_symbol == 1){
      last_element = elem_ptr;
    }
    buff[elem_ptr] = symb_data;

    if ( last_symbol == 1){
      last_symbol_exit: break;
    }
  }
}

void read_block(
  const symb_data_t buff[BUFFER_SIZE],
  ap_uint<BUFFER_ADDR_SIZE> last_element, 
  stream<coder_interf_t> &out  
  ){

  read_block_loop:for (int elem_ptr = last_element; elem_ptr >=0 ; --elem_ptr){
    #pragma HLS LOOP_TRIPCOUNT max=2048
    // #pragma HLS PIPELINE rewind 
    #pragma HLS PIPELINE 
    coder_interf_t symbol;
    symb_data_t symb_data = buff[elem_ptr];
    symb_ctrl_t last_symbol = (elem_ptr == 0)? 1 : 0; 
    out << bits_to_intf(symb_data,last_symbol) ;
  }
}


void output_stack(
  stream<coder_interf_t > &in, 
  stream<coder_interf_t > &out
  ){
  #if OUTPUT_STACK_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=in
  #pragma HLS INTERFACE axis register_mode=both register port=out
  #endif

  #pragma HLS DATAFLOW
  #pragma HLS INTERFACE ap_ctrl_none port=return

  symb_data_t buffers[BUFFER_SIZE];
  #pragma HLS STREAM variable=buffers depth=2 off
  ap_uint<BUFFER_ADDR_SIZE> last_element;
  #pragma HLS dataflow
  write_block(in, buffers, last_element);
  read_block(buffers, last_element,out);
}


