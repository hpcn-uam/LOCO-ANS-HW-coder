
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


typedef axis<ap_uint<OUTPUT_SIZE>,OUTPUT_STACK_BYTES_SIZE,0,0 > TSG_out_intf;


void TSG_coder(
  //input
  stream<coder_interf_t> &in,
  //output
  stream<byte_block<OUT_WORD_BYTES>> &byte_block_stream,
  stream<ap_uint<OUTPUT_STACK_BYTES_SIZE> > &last_byte_idx
  //status registers
  // ap_uint<1> stack_overflow
  );

#endif // TSG_CODER_HPP