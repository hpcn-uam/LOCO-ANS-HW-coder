
#ifndef TSG_CODER_HPP
#define TSG_CODER_HPP


#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <tgmath.h>
#include "../modules/coder_config.hpp"
#include "../modules/input_buffers/src/input_buffers.hpp"
#include "../modules/subsym_gen/src/subsym_gen.hpp"
#include "../modules/ANS_coder/src/ANS_coder.hpp"
#include "../modules/output_stack/src/output_stack.hpp"
#include "../modules/pack_out_bytes/src/pack_out_bytes.hpp"

//metadata: bytes in block,is last block?
typedef ap_uint<OUTPUT_STACK_BYTES_SIZE+1> tsg_blk_metadata;
typedef axis<ap_uint<OUTPUT_SIZE>,0,0,0 > TSG_out_intf;



void TSG_coder(
  //input
  stream<coder_interf_t> &in,
  //output
  stream<TSG_out_intf> &byte_block_stream,
  stream<tsg_blk_metadata> &out_blk_metadata
  //status registers
  // ap_uint<1> stack_overflow
  );


// ap_axis adds tkeep and strobe signals, which I don't think I should be supporting 
// Instead, if a more AXIS compatible interface is needed, the next adapter 
// can be used
void TSG_input_adapter(
  stream<axis<symb_data_t,0,0,0>> &in,
  stream<coder_interf_t> &out){
  #pragma HLS INTERFACE axis register_mode=both register port=in
  #pragma HLS INTERFACE axis register_mode=both register port=out
  
  #pragma HLS PIPELINE style=frp

  START_SW_ONLY_LOOP(! in.empty())
  auto ie= in.read();
  out << bits_to_intf(ie.data,ie.last);
  END_SW_ONLY_LOOP
}

#endif // TSG_CODER_HPP