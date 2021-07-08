
#ifndef TSG_CODER_HPP
#define TSG_CODER_HPP


#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <tgmath.h>
#include "../../coder_config.hpp"
#include "../../input_buffers/src/input_buffers.hpp"
#include "../../subsym_gen/src/subsym_gen.hpp"
#include "../../ANS_coder/src/ANS_coder.hpp"
#include "../../output_stack/src/output_stack.hpp"
#include "../../pack_out_bytes/src/pack_out_bytes.hpp"

//metadata: bytes in block,is last block?

using namespace hls;
void TSG_coder(
  //input
  stream<coder_interf_t> &in,
  //output
  stream<TSG_out_intf> &byte_block_stream,
  stream<tsg_blk_metadata> &out_blk_metadata
  //status registers
  // ap_uint<1> stack_overflow
  );


#endif // TSG_CODER_HPP