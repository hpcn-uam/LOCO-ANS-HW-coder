
#ifndef FILE_WRITER_HPP
#define FILE_WRITER_HPP


#include "../../DMA_engines/src/dma.hpp"
#include "../../../src/TSG_coder.hpp"

void File_writer(
  //inputs
  stream<TSG_out_intf> &in_byte_block_stream,
  stream<tsg_blk_metadata> &in_blk_metadata,
  //outputs
  stream<mem_data>  & out_stream,
  stream<ap_uint<DMA_ADDRESS_RANGE_BITS>> & out_offset,
  stream<ap_uint<NUM_OF_OUT_ELEM_BITS>> & out_num_of_elememts);


#endif // FILE_WRITER_HPP