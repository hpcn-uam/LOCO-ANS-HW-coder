
#ifndef TSG_CODER_HPP
#define TSG_CODER_HPP

#include <hls_stream.h>
#include <tgmath.h>
#include "../modules/coder_config.hpp"
#include "../modules/input_buffers/src/input_buffers.hpp"
#include "../modules/subsym_gen/src/subsym_gen.hpp"
#include "../modules/ANS_coder/src/ANS_coder.hpp"

void TSG_coder(
  stream<coder_interf_t> &in,
  stream<bit_blocks> &bit_block_stream);
#endif // TSG_CODER_HPP