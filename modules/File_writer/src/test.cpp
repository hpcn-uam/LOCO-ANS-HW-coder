



#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>

#include <vector>
#include <list>
#include "File_writer.hpp"

using namespace std;
using namespace hls;

#define NUM_OF_TESTS 2

int main(int argc, char const *argv[])
{
  
  //inputs
  stream<TSG_out_intf> in_byte_block_stream;
  stream<tsg_blk_metadata> in_blk_metadata;
  //outputs
  stream<mem_data>   out_stream;
  stream<ap_uint<DMA_ADDRESS_RANGE_BITS>>  out_offset;
  stream<ap_uint<NUM_OF_OUT_ELEM_BITS>>  out_num_of_elememts;

  int byte_counter = 0 ;
  for(unsigned test_id = 0; test_id < NUM_OF_TESTS; ++test_id) {
    cout<<"Processing test "<<test_id<<endl;
    //generate input
    int block_size = MAX(OUTPUT_STACK_SIZE - test_id*10,1);
    ap_uint<OUTPUT_STACK_BYTES_SIZE> last_byte_idx_elem = block_size-1;
    ap_uint<1> last_block = test_id == NUM_OF_TESTS-1?1:0;
    in_blk_metadata << (last_byte_idx_elem, last_block);
    for(unsigned i = 0; i < block_size; ++i) {
      TSG_out_intf in_elem;
      in_elem.data = i;
      in_elem.last = i ==block_size-1?1:0;
      in_elem.keep = -1;
      in_elem.strb = -1;
      in_byte_block_stream << in_elem;
    }

    //DUT
    File_writer(
    //inputs
      in_byte_block_stream,
      in_blk_metadata,
    //outputs
     out_stream,
     out_offset,
     out_num_of_elememts);


    //check output
    int hw_out_offset = out_offset.read();
    ASSERT(hw_out_offset,==,byte_counter);


    ASSERT(in_byte_block_stream.size(), ==, 0);
    ASSERT(in_blk_metadata.size(), ==, 0);
    ASSERT(out_stream.size(), ==, 0);
    ASSERT(out_offset.size(), ==, 0);
    ASSERT(out_num_of_elememts.size(), ==, 0);
    cout<<"  | SUCCESS"<<endl;

    byte_counter += block_size;

  }
  return 0;
}