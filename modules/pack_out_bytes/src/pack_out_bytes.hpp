
#ifndef PACK_OUT_BYTES_HPP
#define PACK_OUT_BYTES_HPP

#include <hls_stream.h>
#include <tgmath.h>

// #include "ap_axi_sdata.h"
#include "../../coder_config.hpp"
#include "../../ANS_coder/src/ANS_coder.hpp"

using namespace hls;

// Complementary function to byte-align a byte stream
void pack_out_bytes(
  stream<byte_block> &in_bytes,
  stream<byte_block> &out_bytes);

#endif // PACK_OUT_BYTES_HPP