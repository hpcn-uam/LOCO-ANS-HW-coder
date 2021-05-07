
#ifndef SUBSYMB_GEN_HPP
#define SUBSYMB_GEN_HPP

#include <hls_stream.h>
#include <tgmath.h>
#include "../../coder_config.hpp"
#include "../../input_buffers/src/input_buffers.hpp"
// #include "ap_int.h"
// #define AP_INT_MAX_W 16384
// #include "ap_axi_sdata.h"

// #ifdef __SYNTHESIS__
//   #define NDEBUG
// #endif
// #define INPUT_BPP (8)

// #define Z_SIZE (INPUT_BPP-1)
// #define Y_SIZE (1)
// #define THETA_SIZE (5) //32 tables
// #define P_SIZE (5) //32 tables
/*#define PRED_SYMB_SIZE (Z_SIZE + Y_SIZE + THETA_SIZE + P_SIZE)
#define SYMB_DATA_SIZE MAX(INPUT_BPP,PRED_SYMB_SIZE)
#define SYMB_CTRL_SIZE 1

#define IS_COMPRESS_SYMB 0
#define IS_FIRST_PX 1

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

using namespace hls;

// Coder input interface types and conversions
typedef ap_uint<SYMB_DATA_SIZE> symb_data_t;
typedef ap_uint<SYMB_CTRL_SIZE> symb_ctrl_t;
typedef ap_uint<SYMB_DATA_SIZE+SYMB_CTRL_SIZE> coder_interf_t; //

coder_interf_t bits_to_intf(symb_data_t symb,symb_ctrl_t ctrl);
void intf_to_bits(coder_interf_t intf, symb_data_t &symb,symb_ctrl_t &ctrl);
*/

// Coder input data types and conversions
// typedef ap_uint<INPUT_BPP> pixel_t;
/*struct predict_symbol_t{
    ap_uint<Z_SIZE> z;
    ap_uint<Y_SIZE> y;
    ap_uint<THETA_SIZE> theta_id;
    ap_uint<P_SIZE> p_id;

    predict_symbol_t(uint _z,uint _y,uint _theta,uint _p):
                      z(_z),y(_y),theta_id(_theta),p_id(_p){};
    predict_symbol_t(ap_uint<PRED_SYMB_SIZE> bits){
      (p_id,theta_id,y,z) = bits;
    };

    ap_uint<PRED_SYMB_SIZE> to_bits(){
      return (p_id,theta_id,y,z);
    }

    const ap_uint<PRED_SYMB_SIZE>& operator=(const ap_uint<PRED_SYMB_SIZE> &bits){
      (p_id,theta_id,y,z) = bits;
      return bits;
    }
  };*/

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


void sub_symbol_gen(
  stream<coder_interf_t> &in,
  stream<subsymb_t> &symbol_stream
  );


#endif // SUBSYMB_GEN_HPP