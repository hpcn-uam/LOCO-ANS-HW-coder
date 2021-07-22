
#ifndef INPUT_BUFFERS_HPP
#define INPUT_BUFFERS_HPP


#include <hls_stream.h>
#include <tgmath.h>

// #include "ap_axi_sdata.h"
#include "../../coder_config.hpp"

// #ifdef __SYNTHESIS__
//   #define NDEBUG
// #endif

using namespace hls;



coder_interf_t bits_to_intf(symb_data_t symb,symb_ctrl_t ctrl);
void intf_to_bits(coder_interf_t intf, symb_data_t &symb,symb_ctrl_t &ctrl);



void input_buffers(
  stream<coder_interf_t > &in, 
  stream<coder_interf_t > &out,
  stream<ap_uint<1>> &last_block);


#endif // INPUT_BUFFERS_HPP