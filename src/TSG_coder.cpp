
#include "TSG_coder.hpp"



void output_data_interface(
  //inputs
  stream<byte_block<OUT_WORD_BYTES>> &byte_block_stream,
  //output
  stream<TSG_out_intf> &out_data_stream){
  #pragma HLS PIPELINE style=frp

  auto in_data = byte_block_stream.read();

  //translate num_of_bytes to tkeep
  TSG_out_intf out_elem;
  out_elem.data = in_data.data;
  out_elem.keep = in_data.get_byte_mask();
  out_elem.strb = out_elem.keep;
  out_elem.last = in_data.is_last()? 1: 0;
  out_data_stream << out_elem;

}

void output_metadata_interface(
  //input
  stream<ap_uint<OUTPUT_STACK_BYTES_SIZE> > &last_byte_idx,
  stream<ap_uint<1>> &last_block,
  //output
  stream<tsg_blk_metadata> &out_blk_metadata){
  #pragma HLS PIPELINE style=frp
  auto last_block_elem = last_block.read();
  auto last_byte_idx_elem = last_byte_idx.read();

  out_blk_metadata <<(last_byte_idx_elem,last_block_elem);
}

void TSG_coder(
  //input
  stream<coder_interf_t> &in,
  //output
  stream<TSG_out_intf> &byte_block_stream,
  stream<tsg_blk_metadata> &out_blk_metadata
  //status registers
  // ap_uint<1> stack_overflow
  ){

  #pragma HLS INTERFACE axis register_mode=both register port=in
  #pragma HLS INTERFACE axis register_mode=both register port=byte_block_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_blk_metadata
  #pragma HLS INTERFACE ap_ctrl_none port=return
  //status registers
  // #pragma HLS INTERFACE s_axilite port=stack_overflow bundle=control

  #pragma HLS DATAFLOW disable_start_propagation
  
  stream<coder_interf_t> inverted_data;
  #pragma HLS STREAM variable=inverted_data depth=2
  stream<ap_uint<1>> last_block;
  #pragma HLS STREAM variable=last_block depth=32
  input_buffers(in, inverted_data,last_block);


  stream<subsymb_t> symbol_stream;
  #pragma HLS STREAM variable=symbol_stream depth=2
  START_SW_ONLY_LOOP(! inverted_data.empty())
  sub_symbol_gen(inverted_data,symbol_stream);
  END_SW_ONLY_LOOP

  stream<byte_block<OUT_WORD_BYTES>> coded_byte_stream;
  #pragma HLS STREAM variable=coded_byte_stream depth=2
  START_SW_ONLY_LOOP(!symbol_stream.empty())
  ANS_coder(symbol_stream,coded_byte_stream);
  END_SW_ONLY_LOOP

  ap_uint<1> stack_overflow;
  stream<byte_block<OUT_WORD_BYTES>> inverted_byte_block;
  #pragma HLS STREAM variable=inverted_byte_block depth=2
  stream<ap_uint<OUTPUT_STACK_BYTES_SIZE>,2 > last_byte_idx;
  output_stack(coded_byte_stream,inverted_byte_block,last_byte_idx, stack_overflow);

  //Error handler: TODO
    // read "end of symbol stream", stack_overflow (other warning and error signals)
    // generate a single signal stating the war/error registars need to be checked
 
  //output interface
  START_SW_ONLY_LOOP(!inverted_byte_block.empty())
  output_data_interface(inverted_byte_block,byte_block_stream);
  END_SW_ONLY_LOOP

  START_SW_ONLY_LOOP(!last_byte_idx.empty() || !last_block.empty())
  ASSERT(last_byte_idx.empty(),==,last_block.empty())
  output_metadata_interface(last_byte_idx,last_block,out_blk_metadata);
  END_SW_ONLY_LOOP

  // pack_out_bytes(inverted_byte_block,byte_block_stream);
}
