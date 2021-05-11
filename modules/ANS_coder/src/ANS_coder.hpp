
#ifndef ANS_CODER_HPP
#define ANS_CODER_HPP

#include <hls_stream.h>
#include <tgmath.h>
#include "../../coder_config.hpp"
#include "../../subsym_gen/src/subsym_gen.hpp"
#include "../../input_buffers/src/input_buffers.hpp"

/*#define MAX_ANS_SYMB_SIZE 4 // MAX(C) = 8
#define SUBSYMB_SIZE MAX(MAX_ANS_SYMB_SIZE,INPUT_BPP)
#define SUBSYMB_SIZE_BITS 4 // int(log2(SUBSYMB_SIZE))
#define SUBSYMB_INFO_SIZE MAX( MAX(SUBSYMB_SIZE_BITS,THETA_SIZE),P_SIZE)
// #define SUBSYMB_INFO_SIZE MAX( MAX(int(log2(SUBSYMB_SIZE)),THETA_SIZE),P_SIZE)

#define SUBSYMB_Y 0
#define SUBSYMB_Z 1
#define SUBSYMB_BYPASS 2
#define SUBSYMB_Z_LAST 3

struct subsymb_t{
    ap_uint<SUBSYMB_SIZE> subsymb;
    ap_uint<2> type;
    ap_uint<SUBSYMB_INFO_SIZE> info; // number of bits subsymb contains (for bypass type) // param if z or y
    ap_uint<1> end_of_block;

  };*/


struct tANS_table_t {
  ap_uint<NUM_ANS_BITS> state;
  ap_uint<LOG2_ANS_BITS> bits;
} ;


struct bit_blocks {
  ap_uint<BIT_BLOCK_SIZE> data;
  ap_uint<LOG2_BIT_BLOCK_SIZE> bits;
  ap_uint<1> last_block;
  // block_type;
} ;

void ANS_coder(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks> &bit_block_stream);


#endif // ANS_CODER_HPP