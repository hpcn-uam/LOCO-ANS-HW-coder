
#ifndef INPUT_BUFFERS_HPP
#define INPUT_BUFFERS_HPP


#include <hls_stream.h>
#include <tgmath.h>

// #include "ap_axi_sdata.h"
#include "../../coder_config.hpp"

// #ifdef __SYNTHESIS__
//   #define NDEBUG
// #endif

#define PRED_SYMB_SIZE (Z_SIZE + Y_SIZE + THETA_SIZE + P_SIZE)
#define SYMB_DATA_SIZE MAX(INPUT_BPP,PRED_SYMB_SIZE)
#define SYMB_CTRL_SIZE 1


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

using namespace hls;

// Coder input interface types and conversions
typedef ap_uint<SYMB_DATA_SIZE> symb_data_t;
typedef ap_uint<SYMB_CTRL_SIZE> symb_ctrl_t;
typedef ap_uint<SYMB_DATA_SIZE+SYMB_CTRL_SIZE> coder_interf_t; //

coder_interf_t bits_to_intf(symb_data_t symb,symb_ctrl_t ctrl);
void intf_to_bits(coder_interf_t intf, symb_data_t &symb,symb_ctrl_t &ctrl);



void input_buffers(stream<coder_interf_t > &in, stream<coder_interf_t > &out);


#endif // INPUT_BUFFERS_HPP