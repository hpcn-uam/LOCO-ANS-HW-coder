
#ifndef FILE_WRITER_HPP
#define FILE_WRITER_HPP

#include "../../coder_config.hpp"
#include "../../DMA_engines/src/dma.hpp"

using namespace hls;

constexpr unsigned int OFFSET_INIT = 5;

void File_writer(
  //inputs
  stream<TSG_out_intf> &in_byte_block_stream,
  stream<tsg_blk_metadata> &in_blk_metadata,
  stream<px_t> &first_px_stream,
  //outputs
  stream<odma_data>  & out_stream,
  stream<odma_command> & out_command);


#endif // FILE_WRITER_HPP