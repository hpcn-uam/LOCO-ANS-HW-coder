
#include "input_buffers.h"


void write_block(stream<coder_interf_t> &in, symb_data_t buff[BUFFER_SIZE],ap_uint<1> &is_first_block){
// void write_block(stream<coder_interf_t> &in, coder_interf_t &buff[2], stream<apt_uint<1>>  ){
  // static ap_uint<1> buffer_ptr=0;

  write_block_loop:for (int elem_ptr = 0; elem_ptr < BUFFER_SIZE; ++elem_ptr){
    coder_interf_t symbol = in.read();
    if (elem_ptr == 0 ){
      is_first_block = symbol.is_first_px();
    }
    buff[elem_ptr] = symbol.get_data();
  }
}

void read_block(const symb_data_t buff[BUFFER_SIZE],ap_uint<1> &is_first_block, stream<coder_interf_t> &out  ){
// void read_block(const coder_interf_t buff[2]BUFFER_SIZE], stream<coder_interf_t> &out  ){
  // static ap_uint<1> buffer_ptr=0;
  read_block_loop:for (int elem_ptr = BUFFER_SIZE-1; elem_ptr >=0 ; --elem_ptr){
    coder_interf_t symbol;
    if(elem_ptr == 0) {
      symbol.set_first_px_flag (is_first_block);
    }else{
      symbol.set_first_px_flag (0);
    }
    symbol.set_data(buff[elem_ptr]);
    out.write(symbol);
    // buff[buffer_ptr][elem_ptr] = in.read();
  }
}

void input_buffers(stream<coder_interf_t > &in, stream<coder_interf_t > &out){
// void input_buffers(stream<coder_interf_t> &in, stream<coder_interf_t> &out){
#pragma HLS INTERFACE axis register_mode=both register port=out
#pragma HLS INTERFACE axis register_mode=both register port=in
#pragma HLS INTERFACE ap_ctrl_none port=return
  symb_data_t buffers[BUFFER_SIZE];
  ap_uint<1> is_first_block;
  #pragma HLS dataflow
  write_block(in, buffers, is_first_block);
  read_block(buffers,is_first_block, out);

}
