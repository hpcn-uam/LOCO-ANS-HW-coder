
#include "input_buffers.h"

coder_interf_t bits_to_intf(symb_data_t symb,symb_ctrl_t ctrl){
  return (ctrl,symb);
}

void intf_to_bits(coder_interf_t intf, symb_data_t &symb,symb_ctrl_t &ctrl){
  (ctrl,symb) = intf;
}

void write_block(
  stream<coder_interf_t> &in, 
  symb_data_t buff[BUFFER_SIZE],
  ap_uint<1> &is_first_block
  ){
#pragma HLS INLINE
  write_block_loop:for (int elem_ptr = 0; elem_ptr < BUFFER_SIZE; ++elem_ptr){
  #pragma HLS PIPELINE rewind enable_flush
    symb_data_t symb_data;
    symb_ctrl_t symb_ctrl;
    intf_to_bits(in.read(),symb_data,symb_ctrl);

    if (elem_ptr == 0 ){
      is_first_block = symb_ctrl;
    }
    buff[elem_ptr] = symb_data;
  }
}

void read_block(
  const symb_data_t buff[BUFFER_SIZE],
  ap_uint<1> &is_first_block, 
  stream<ap_uint<SYMB_DATA_SIZE+2> > &out  
  ){

  read_block_loop:for (int elem_ptr = BUFFER_SIZE-1; elem_ptr >=0 ; --elem_ptr){
    #pragma HLS PIPELINE rewind enable_flush
    coder_interf_t symbol;
    symb_data_t symb_data = buff[elem_ptr];
    // symb_ctrl = (end of block, is first pixel )
    ap_uint<1> is_first_px; 
    ap_uint<1> end_of_block; 
    if(elem_ptr == 0) {
      is_first_px = is_first_block;
      end_of_block = 1; 
    }else{
      is_first_px = 0;
      end_of_block = 0;
    }
    out << (symb_data,end_of_block,is_first_px) ;
  }
}


void input_buffers(
  stream<coder_interf_t > &in, 
  stream<ap_uint<SYMB_DATA_SIZE+2> > &out
  ){
#pragma HLS INTERFACE axis register_mode=both register port=out
#pragma HLS INTERFACE axis register_mode=both register port=in
#pragma HLS INTERFACE ap_ctrl_none port=return
  symb_data_t buffers[BUFFER_SIZE];
  ap_uint<1> is_first_block;
  #pragma HLS dataflow
  write_block(in, buffers, is_first_block);
  read_block(buffers, is_first_block,out);
}