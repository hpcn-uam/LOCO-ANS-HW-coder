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

#ifndef OUTPUT_STACK_HPP
#define OUTPUT_STACK_HPP


#include <hls_stream.h>
#include <tgmath.h>

// #include "ap_axi_sdata.h"
#include "../../coder_config.hpp"
#include "../../ANS_coder/src/ANS_coder.hpp"

using namespace hls;

// This function assumes a packed stream of byte_block
// That is, only the last byte_block can have bytes != OUT_WORD_BYTES
// This is to avoid storing the bytes field for every byte_block, optimizing
// memory resources and to have a most efficient use of the stack memory. 
// The former reason is not always true as BRAM bits may not be used. 
// E.g. 32 bit words in Xilinx 36K BRAMS. The byte field could be stored in the 
// extra 4 bits
void output_stack(
  stream<byte_block<OUT_WORD_BYTES> > &in, 
  stream<byte_block<OUT_WORD_BYTES> > &out,
  stream<ap_uint<OUTPUT_STACK_BYTES_SIZE> > &last_byte_idx,
  ap_uint<1> &stack_overflow);

#endif // OUTPUT_STACK_HPP