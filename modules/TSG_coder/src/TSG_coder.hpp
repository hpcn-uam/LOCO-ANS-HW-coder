
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

// #define EXTERNAL_ANS_ROM

using namespace hls;
void TSG_coder(
  //input
  stream<coder_interf_t> &in,
  //output
  stream<TSG_out_intf> &byte_block_stream,
  stream<tsg_blk_metadata> &out_blk_metadata
  #ifdef EXTERNAL_ANS_ROM
  ,
  const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2],
  const tANS_table_t  tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]
  #endif

  //status registers
  // ap_uint<1> stack_overflow
  );



void TSG_coder_double_lane(
  //Lane 0 
    //input
    stream<coder_interf_t> &in_0,
    //output
    stream<TSG_out_intf> &byte_block_stream_0,
    stream<tsg_blk_metadata> &out_blk_metadata_0,
  //Lane 1
    //input
    stream<coder_interf_t> &in_1,
    //output
    stream<TSG_out_intf> &byte_block_stream_1,
    stream<tsg_blk_metadata> &out_blk_metadata_1
  //status registers
  // ap_uint<1> stack_overflow
  );

#endif // TSG_CODER_HPP