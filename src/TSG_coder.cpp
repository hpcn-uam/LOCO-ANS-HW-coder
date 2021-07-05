
#include "TSG_coder.hpp"



void output_interface(
  stream<byte_block<OUT_WORD_BYTES>> &byte_block_stream,
  stream<ap_uint<OUTPUT_STACK_BYTES_SIZE> > &last_byte_idx,
  stream<TSG_out_intf> out_stream){


  // put the last byte idx in tuser
  //translate num_of_bytes to tkeep

}

void TSG_coder(
  //input
  stream<coder_interf_t> &in,
  //output
  stream<byte_block<OUT_WORD_BYTES>> &byte_block_stream,
  stream<ap_uint<OUTPUT_STACK_BYTES_SIZE> > &last_byte_idx
  //status registers
  // ap_uint<1> stack_overflow
  ){

  #pragma HLS INTERFACE axis register_mode=both register port=in
  #pragma HLS INTERFACE axis register_mode=both register port=byte_block_stream
  #pragma HLS INTERFACE ap_ctrl_none port=return
  //status registers
  #pragma HLS INTERFACE s_axilite port=stack_overflow bundle=control

  #pragma HLS DATAFLOW disable_start_propagation
  
  stream<coder_interf_t> inverted_data;
  input_buffers(in, inverted_data);


  stream<subsymb_t> symbol_stream;
  #ifdef __SYNTHESIS__
    sub_symbol_gen(inverted_data,symbol_stream);
  #else
    while (! inverted_data.empty()){
      sub_symbol_gen(inverted_data,symbol_stream);
    }
  #endif

  stream<byte_block<OUT_WORD_BYTES>> coded_byte_stream;
  #ifdef __SYNTHESIS__
    ANS_coder(symbol_stream,coded_byte_stream);
  #else
    while (!symbol_stream.empty()){
      ANS_coder(symbol_stream,coded_byte_stream);
    } 
  #endif

  ap_uint<1> stack_overflow;// TODO: set it as an output interrupt
  output_stack(coded_byte_stream,byte_block_stream,last_byte_idx, stack_overflow);
  // stream<byte_block<OUT_WORD_BYTES>> inverted_byte_block;
  // output_stack(coded_byte_stream,inverted_byte_block,last_byte_idx, stack_overflow);

  //Error handler
    // read "end of symbol stream", stack_overflow (other warning and error signals)
    // generate a single signal stating the war/error registars need to be checked
 
  //output interface
  

  // pack_out_bytes(inverted_byte_block,byte_block_stream);
}
