
#ifndef SUBSYMB_GEN_HPP
#define SUBSYMB_GEN_HPP

#include <hls_stream.h>
#include <tgmath.h>
#include "../../coder_config.hpp"
#include "../../input_buffers/src/input_buffers.hpp"


#define MAX_ANS_SYMB_SIZE 4 // MAX(C) = 8
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


  };

void subsymbol_gen(
  stream<coder_interf_t> &in,
  stream<subsymb_t> &symbol_stream
  );


#endif // SUBSYMB_GEN_HPP