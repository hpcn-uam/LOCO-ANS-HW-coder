
#ifndef ANS_CODER_HPP
#define ANS_CODER_HPP

#include <hls_stream.h>
#include <tgmath.h>
#include "../../coder_config.hpp"
#include "../../subsym_gen/src/subsym_gen.hpp"
#include "../../input_buffers/src/input_buffers.hpp"


struct tANS_table_t {
  ap_uint<NUM_ANS_BITS> state;
  ap_uint<LOG2_ANS_BITS> bits;
} ;



struct bit_blocks {
  // static constexpr size_t _LOG2_BIT_BLOCK_SIZE = size_t(ceil(log2(BIT_BLOCK_SIZE))); gcc allows this
  ap_uint<BIT_BLOCK_SIZE> data;
  ap_uint<LOG2_BIT_BLOCK_SIZE> bits;
  // ap_uint<_LOG2_BIT_BLOCK_SIZE> bits;
  ap_uint<1> last_block;

  /*  template<size_t WMeta>
  inline bit_blocks& operator=(const bit_blocks_with_meta<WMeta> &in){
    data = in.data;
    bits = in.bits;
    last_block = in.last_block;
    return *this;
  }*/
} ;

template <size_t WMeta>
struct bit_blocks_with_meta : public bit_blocks{
  ap_uint<WMeta> metadata;

} ;

void ANS_coder(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks> &bit_block_stream);


/*void code_symbols_loop(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks> &bit_block_stream);
*/

void code_symbols(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks_with_meta<NUM_ANS_BITS>> &out_bit_stream);

void ANS_output(
    stream<bit_blocks_with_meta<NUM_ANS_BITS>> &in_bit_blocks,
    stream<bit_blocks> &out_bit_blocks);


#endif // ANS_CODER_HPP