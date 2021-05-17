
#ifndef OUTPUT_STACK_HPP
#define OUTPUT_STACK_HPP


#include <hls_stream.h>
#include <tgmath.h>

// #include "ap_axi_sdata.h"
#include "../../coder_config.hpp"
#include "../../ANS_coder/src/ANS_coder.hpp"

using namespace hls;

void output_stack_sw(
  stream<byte_block > &in, 
  stream<byte_block > &out);


#endif // OUTPUT_STACK_HPP