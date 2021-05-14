
#ifndef ANS_CODER_HPP
#define ANS_CODER_HPP

#include <hls_stream.h>
#include <tgmath.h>
#include "../../coder_config.hpp"
#include "../../subsym_gen/src/subsym_gen.hpp"
#include "../../input_buffers/src/input_buffers.hpp"

// #define DEBUG

struct tANS_table_t {
  ap_uint<NUM_ANS_BITS> state;
  ap_uint<LOG2_ANS_BITS> bits;
} ;

struct bit_blocks {
  // gcc allows the following commented declaration, VITIS HLS doesn't 
  // static constexpr size_t _LOG2_BIT_BLOCK_SIZE = size_t(ceil(log2(BIT_BLOCK_SIZE)));
  ap_uint<BIT_BLOCK_SIZE> data;
  ap_uint<LOG2_BIT_BLOCK_SIZE> bits;
  // ap_uint<_LOG2_BIT_BLOCK_SIZE> bits;
  ap_uint<1> last_block;
} ;

template <size_t WMeta>
struct bit_blocks_with_meta : public bit_blocks{
  ap_uint<WMeta> metadata;
} ;

typedef ap_uint<OUTPUT_SIZE> out_word_t;

void ANS_coder(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks> &bit_block_stream);

void code_symbols(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks_with_meta<NUM_ANS_BITS>> &out_bit_stream);

void serialize_last_state(
    stream<bit_blocks_with_meta<NUM_ANS_BITS>> &in_bit_blocks,
    stream<bit_blocks> &out_bit_blocks);


#endif // ANS_CODER_HPP