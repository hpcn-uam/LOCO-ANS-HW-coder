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
  //outputs
  stream<odma_data>  & out_stream,
  stream<odma_command> & out_command){

  #pragma HLS INTERFACE axis register_mode=both register port=in_byte_block_stream
  #pragma HLS INTERFACE axis register_mode=both register port=int_blk_metadata
  #pragma HLS INTERFACE axis register_mode=both register port=out_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_command
  #pragma HLS INTERFACE ap_ctrl_none port=return


  static ap_uint<DMA_ADDRESS_RANGE_BITS> offset = OFFSET_INIT;
  //static odma_data out_buffer=0; 
  //static ap_uint<> buffer_ptr=0 ;
  ap_uint<OUTPUT_STACK_BYTES_SIZE> last_byte_idx_elem;
  ap_uint<1> last_block_elem;
  (last_byte_idx_elem,last_block_elem) = in_blk_metadata.read();

  BUILD_BUG_ON(odma_data::width != 8); // only working for 1 byte output

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
    out_command << (decltype(offset)(0),decltype(num_of_elements)(4),ap_uint<1>(1) );
    auto bytes_sent = offset - OFFSET_INIT;
    for(unsigned i = 0; i < 4; ++i) {
      out_stream << (bytes_sent>>(i*8)); //store in little endian
    }

    offset = OFFSET_INIT;
    //out_buffer =0; 
  //static ap_uint<> buffer_ptr =0;
  }
}
