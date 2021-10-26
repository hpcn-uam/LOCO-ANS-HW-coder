/*
  Copyright 2021 Tobías Alonso, Autonomous University of Madrid
  This file is part of LOCO-ANS HW encoder.
  LOCO-ANS HW encoder is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  LOCO-ANS HW encoder is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with LOCO-ANS HW encoder.  If not, see <https://www.gnu.org/licenses/>.
*/

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


/*
WARNING: 
To optimize space, the current implementation doesn't allow
remainder_reduct bits to change within a code block.
The last element's remainder_reduct is applied to all elements
*/
void input_buffers(
  stream<coder_interf_t > &in, 
  stream<coder_interf_t > &out,
  stream<ap_uint<1>> &last_block);


#endif // INPUT_BUFFERS_HPP