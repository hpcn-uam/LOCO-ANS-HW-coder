
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
  stream<odma_data>   out_stream;
  stream<odma_command>  out_command;
  std::list<odma_data> in_list;
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

      in_list.push_back(i);
    }

    //DUT
    File_writer(
    //inputs
      in_byte_block_stream,
      in_blk_metadata,
    //outputs
     out_stream,
     out_command);


    //check output
    ap_uint<DMA_ADDRESS_RANGE_BITS> cmd_off ;
    ap_uint<NUM_OF_OUT_ELEM_BITS> cmd_num_of_elememts; 
    ap_uint<1> cmd_last; 

    (cmd_off,cmd_num_of_elememts,cmd_last)  = out_command.read();

    ASSERT(cmd_off,==,byte_counter+OFFSET_INIT);
    ASSERT(cmd_num_of_elememts,==,block_size);
    ASSERT(cmd_last,==,0);

    for(unsigned elem_idx = 0; elem_idx < block_size; ++elem_idx) {
      odma_data out_elem = out_stream.read();
      odma_data golden_data = in_list.front();
      in_list.pop_front();

      ASSERT(out_elem,==,golden_data,"elem_idx: "<<elem_idx);
    }

    byte_counter += block_size;

    if(last_block==1) {
      (cmd_off,cmd_num_of_elememts,cmd_last)  = out_command.read();
      ASSERT(cmd_off,==,0);
      ASSERT(cmd_num_of_elememts,==,OFFSET_INIT);
      ASSERT(cmd_last,==,1);

      for(unsigned elem_idx = 0; elem_idx < OFFSET_INIT; ++elem_idx) {
        odma_data out_elem = out_stream.read();
        odma_data golden_data = (byte_counter>>(elem_idx*8)) & 0xFF;

        ASSERT(out_elem,==,golden_data,"elem_idx: "<<elem_idx);
      }
    }

    ASSERT(in_byte_block_stream.size(), ==, 0);
    ASSERT(in_blk_metadata.size(), ==, 0);
    ASSERT(out_stream.size(), ==, 0);
    ASSERT(out_command.size(), ==, 0);
    cout<<"  | SUCCESS"<<endl;



  }
  return 0;
}
