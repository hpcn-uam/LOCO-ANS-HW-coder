#include "File_writer.hpp"

// it sends twise words shared between block.
// In that way, the new block is available in memory inmediatelly
// and next block need to be only byte aligned (not word aligned)
void File_writer(
  //inputs
  stream<TSG_out_intf> &in_byte_block_stream,
  stream<tsg_blk_metadata> &in_blk_metadata,
  //outputs
  stream<mem_data>  & out_stream,
  stream<ap_uint<DMA_ADDRESS_RANGE_BITS>> & out_offset,
  stream<ap_uint<NUM_OF_OUT_ELEM_BITS>> & out_num_of_elememts){

  #pragma HLS INTERFACE axis register_mode=both register port=in_byte_block_stream
  #pragma HLS INTERFACE axis register_mode=both register port=int_blk_metadata
  #pragma HLS INTERFACE axis register_mode=both register port=out_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_offset
  #pragma HLS INTERFACE axis register_mode=both register port=out_num_of_elememts
  #pragma HLS INTERFACE ap_ctrl_none port=return

  static ap_uint<DMA_ADDRESS_RANGE_BITS> offset = 0;
  //static mem_data out_buffer=0; 
  //static ap_uint<> buffer_ptr=0 ;
  ap_uint<OUTPUT_STACK_BYTES_SIZE> last_byte_idx_elem;
  ap_uint<1> last_block_elem;
  (last_byte_idx_elem,last_block_elem) = in_blk_metadata.read();

  BUILD_BUG_ON(mem_data::width != 8); // only working for 1 byte output

  // send commands to DMA
  ap_uint<OUTPUT_STACK_BYTES_SIZE+1> num_of_bytes = (decltype(num_of_bytes))(last_byte_idx_elem) +1;
  decltype(num_of_bytes) num_of_elements = num_of_bytes; // divide (with shift) by output size

  out_offset<< offset;
  out_num_of_elememts << num_of_elements;

  
  // Send data
  File_writer_blk_loop: for( auto bytes_left = num_of_bytes; bytes_left != 0; --bytes_left) {
    //pack out bytes
    TSG_out_intf in_elem = in_byte_block_stream.read();
    out_stream << in_elem.data;
  }

  //update File writer state
  if(last_block_elem == 1) {
    offset = 0;
    //out_buffer =0; 
  //static ap_uint<> buffer_ptr =0;
  }else{
    offset += num_of_elements;
  }
}