
#ifndef OUTPUT_STACK_HPP
#define OUTPUT_STACK_HPP


#include <hls_stream.h>
#include <tgmath.h>

// #include "ap_axi_sdata.h"
#include "../../coder_config.hpp"
#include "../../ANS_coder/src/ANS_coder.hpp"

using namespace hls;

// This function assumes a packed stream of byte_block
// That is, only the last byte_block can have bytes != OUT_WORD_BYTES
// This is to avoid storing the bytes field for every byte_block, optimizing
// memory resources.
// This is not always the case as BRAM bits may not be used. 
// E.g. 32 bit words in Xilinx 36K BRAMS. The byte field could be stored in the 
// extra 4 bits
void output_stack(
  stream<byte_block > &in, 
  stream<byte_block > &out,
  ap_uint<1> &stack_overflow);


#endif // OUTPUT_STACK_HPP