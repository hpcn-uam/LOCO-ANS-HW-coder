
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

#define Z_META_STREAM_SIZE (LOG2_Z_SIZE+CARD_BITS+ANS_SUBSYMBOL_BITS+ 1+ 2*Z_SIZE+THETA_SIZE+1)

struct subsymb_t{
    ap_uint<SUBSYMB_SIZE> subsymb;
    ap_uint<2> type;
    ap_uint<SUBSYMB_INFO_SIZE> info; // number of bits subsymb contains (for bypass type) // param if z or y
    ap_uint<1> end_of_block;

  friend bool operator==(const subsymb_t& l,const subsymb_t& r){
    return (l.subsymb == r.subsymb )&&
            (l.type == r.type )&&
            (l.info == r.info )&&
            (l.end_of_block == r.end_of_block );
  }
};

void subsymbol_gen(
  stream<coder_interf_t> &in,
  stream<subsymb_t> &symbol_stream
  );


class SubSymbolGenerator {
public:
  ap_uint<Z_SIZE> module_reminder;
  ap_uint<1> input_select;
private:
  ap_uint<ANS_SUBSYMBOL_BITS> ans_symb;
  ap_uint<CARD_BITS> encoder_cardinality;
  ap_uint<THETA_SIZE> theta_id;
  ap_uint<1> end_of_block;
  ap_uint<1> escape_symbol_flag;

  //serialize_symbols
public:
  SubSymbolGenerator():input_select(0),module_reminder(0)
  {};

  void z_decompose_post(
    stream<ap_uint <Z_META_STREAM_SIZE> > &z_stream_with_meta,
    stream<subsymb_t> &z_decomposed);

  void serialize_symbols(
    stream<ap_uint <Y_SIZE+P_SIZE> > &y_stream,
    stream<subsymb_t > &z_decomposed,
    stream<subsymb_t> &symbol_stream);

  void transform(
    stream<coder_interf_t> &in,
    stream<subsymb_t> &symbol_stream);
};

void subsymbol_gen_double_lane(
  stream<coder_interf_t> &in_0,
  stream<subsymb_t> &symbol_stream_0,
  stream<coder_interf_t> &in_1,
  stream<subsymb_t> &symbol_stream_1);

#endif // SUBSYMB_GEN_HPP