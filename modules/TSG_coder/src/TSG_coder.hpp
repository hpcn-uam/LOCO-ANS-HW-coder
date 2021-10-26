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

#ifndef TSG_CODER_HPP
#define TSG_CODER_HPP


#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <tgmath.h>
#include "../../coder_config.hpp"
#include "../../input_buffers/src/input_buffers.hpp"
#include "../../subsym_gen/src/subsym_gen.hpp"
#include "../../ANS_coder/src/ANS_coder.hpp"
#include "../../output_stack/src/output_stack.hpp"
#include "../../pack_out_bytes/src/pack_out_bytes.hpp"

//metadata: bytes in block,is last block?

// #define EXTERNAL_ANS_ROM

using namespace hls;
void TSG_coder(
  //input
  stream<coder_interf_t> &in,
  //output
  stream<TSG_out_intf> &byte_block_stream,
  stream<tsg_blk_metadata> &out_blk_metadata
  #if defined(EXTERNAL_ANS_ROM) && !defined(USE_TSG_INTERNAL_ROM)
  ,
  const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2],
  const tANS_table_t  tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]
  #endif

  //status registers
  // ap_uint<1> stack_overflow
  );



void TSG_coder_double_lane(
  //Lane 0 
    //input
    stream<coder_interf_t> &in_0,
    //output
    stream<TSG_out_intf> &byte_block_stream_0,
    stream<tsg_blk_metadata> &out_blk_metadata_0,
  //Lane 1
    //input
    stream<coder_interf_t> &in_1,
    //output
    stream<TSG_out_intf> &byte_block_stream_1,
    stream<tsg_blk_metadata> &out_blk_metadata_1
  //status registers
  // ap_uint<1> stack_overflow
  );

#endif // TSG_CODER_HPP