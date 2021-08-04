
#include "input_buffers.hpp"

coder_interf_t bits_to_intf(symb_data_t symb,symb_ctrl_t ctrl){
  return (ctrl,symb);
}

void intf_to_bits(coder_interf_t intf, symb_data_t &symb,symb_ctrl_t &ctrl){
  (ctrl,symb) = intf;
}

void write_block(
  stream<coder_interf_t> &in, 
  symb_data_t buff[BUFFER_SIZE],
  ap_uint<BUFFER_ADDR_SIZE> &last_element,
  stream<ap_uint<1>> &last_block,
  ap_uint<REM_REDUCT_SIZE> &blk_remainder_reduct){

  write_block_loop:for (int elem_ptr = 0; elem_ptr < BUFFER_SIZE ; ++elem_ptr){
  #pragma HLS LOOP_TRIPCOUNT max=BUFFER_SIZE
  #pragma HLS PIPELINE II=1
    symb_data_t symb_data;
    ap_uint<1> last_symbol;
    ap_uint<REM_REDUCT_SIZE> in_remainder_reduct;
    (last_symbol,in_remainder_reduct,symb_data) = in.read();
    // last_element = elem_ptr;
    if (elem_ptr == BUFFER_SIZE-1 || last_symbol == 1){
      last_element = elem_ptr;
      blk_remainder_reduct = in_remainder_reduct;
      last_block << (last_symbol == 1? 1 : 0);
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
  ap_uint<REM_REDUCT_SIZE> blk_remainder_reduct,
  stream<coder_interf_t> &out ){

  read_block_loop:for (int elem_ptr = last_element; elem_ptr >=0 ; --elem_ptr){
    #pragma HLS LOOP_TRIPCOUNT max=BUFFER_SIZE
    #pragma HLS PIPELINE II=1
    coder_interf_t symbol;
    symb_data_t symb_data = buff[elem_ptr];
    ap_uint<REM_REDUCT_SIZE> in_remainder_reduct= blk_remainder_reduct;
    ap_uint<1> last_symbol = (elem_ptr == 0)? 1 : 0; 
    out << (last_symbol,in_remainder_reduct,symb_data) ;
    // out << bits_to_intf(symb_data,last_symbol) ;
  }
}


void input_buffers(
  stream<coder_interf_t > &in, 
  stream<coder_interf_t > &out,
  stream<ap_uint<1>> &last_block){
  #if INPUT_BUFFERS_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=in
  #pragma HLS INTERFACE axis register_mode=both register port=out
  #pragma HLS INTERFACE axis register_mode=both register port=last_block
  #endif

  #pragma HLS DATAFLOW
  #pragma HLS INTERFACE ap_ctrl_none port=return


  symb_data_t buffers[BUFFER_SIZE];
  #pragma HLS STREAM variable=buffers depth=2 off //TODO: type=pipo
  ap_uint<REM_REDUCT_SIZE> blk_remainder_reduct;
  #pragma HLS STREAM variable=blk_remainder_reduct depth=2 off //TODO: type=pipo

  ap_uint<BUFFER_ADDR_SIZE> last_element;
  write_block(in, buffers, last_element,last_block,blk_remainder_reduct);
  read_block(buffers, last_element,blk_remainder_reduct,out);

}
