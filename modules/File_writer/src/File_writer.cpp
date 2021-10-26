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
#include "File_writer.hpp"


// TODO if out dma bytes >1:
// words shared between blocks should be sent twice.
//   first with the block N last bytes, and them with block N last bytes
//   plus block (N+1) first bytes
// In that way, the new block is available in memory immediately
// and next block can be byte aligned (not word aligned)
void File_writer(
  //inputs
  stream<TSG_out_intf> &in_byte_block_stream,
  stream<tsg_blk_metadata> &in_blk_metadata,
  stream<px_t> &first_px_stream,
  //outputs
  stream<odma_data>  & out_stream,
  stream<odma_command> & out_command){

  #pragma HLS INTERFACE axis register_mode=both register port=in_byte_block_stream
  #pragma HLS INTERFACE axis register_mode=both register port=in_blk_metadata
  #pragma HLS INTERFACE axis register_mode=both register port=first_px_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_command
  #pragma HLS INTERFACE ap_ctrl_none port=return

  #pragma HLS PIPELINE style=flp 

  static enum State {READ_FIRST_PX =0,READ_BINARY_BLOCKS} Writer_state = READ_FIRST_PX;
  #pragma HLS reset variable=Writer_state

  static px_t first_px; 
  static ap_uint<DMA_ADDRESS_RANGE_BITS> offset = OFFSET_INIT;
  // #pragma HLS reset variable=offset
  //static odma_data out_buffer=0; 
  //static ap_uint<> buffer_ptr=0 ;

  BUILD_BUG_ON(odma_data::width != 8); // only working for 1 byte output

  // START_SW_ONLY_LOOP(!first_px_stream.empty() || !in_blk_metadata.empty())

  switch(Writer_state){
    case READ_FIRST_PX:
      first_px = first_px_stream.read();
      offset = OFFSET_INIT;
      Writer_state =  READ_BINARY_BLOCKS;
      break;

    case READ_BINARY_BLOCKS:
      ap_uint<OUTPUT_STACK_BYTES_SIZE> last_byte_idx_elem;
      ap_uint<1> last_block_elem;
      (last_byte_idx_elem,last_block_elem) = in_blk_metadata.read();

      // send command to DMA
      ap_uint<OUTPUT_STACK_BYTES_SIZE+1> num_of_bytes = (decltype(num_of_bytes))(last_byte_idx_elem) +1;
      ap_uint<NUM_OF_OUT_ELEM_BITS> num_of_elements = num_of_bytes; // divide (with shift) by output size

      out_command << (offset,num_of_elements,ap_uint<1>(0));

      // Send data
      File_writer_blk_loop: for( auto bytes_left = num_of_bytes; bytes_left != 0; --bytes_left) {
        #pragma HLS PIPELINE
        //pack out bytes
        TSG_out_intf in_elem = in_byte_block_stream.read();
        out_stream << in_elem.data;
      }

      offset += num_of_elements;

      //update File writer state
      if(last_block_elem == 1) {
        //store bytes in complete block
        out_command << (decltype(offset)(0),decltype(num_of_elements)(OFFSET_INIT),ap_uint<1>(1) );
        auto bytes_sent = offset +1 - OFFSET_INIT ;
        for(unsigned i = 0; i < 4; ++i) {
          out_stream << (bytes_sent>>(i*8)); //store in little endian
        }
        out_stream << first_px;
        // offset = OFFSET_INIT;
        Writer_state =  READ_FIRST_PX;

        //out_buffer =0; 
      //static ap_uint<> buffer_ptr =0;
      }
      break;
  }

  // END_SW_ONLY_LOOP

}

