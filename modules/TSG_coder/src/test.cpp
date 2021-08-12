
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include "TSG_coder.hpp"
#include "../../test/test.hpp"
#include <vector>
#include <list>

using namespace std;
using namespace hls;

#define TEST_BUFFER_SIZE 2048

// #define DEBUG
// #define USE_EXTERNAL_INPUT_VECTOR

#ifdef USE_EXTERNAL_INPUT_VECTOR 
  #define EXT_VECTOR_SIZE (99)
  const long unsigned ext_input_vector[EXT_VECTOR_SIZE]={
    #include "../../LOCO_decorrelator/hw_out_file.dat"
  };
  #define NUM_OF_BLCKS (1)
#else
  #define NUM_OF_BLCKS (9)
#endif


static const tANS_table_t 
  tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
  #include "../../ANS_tables/tANS_y_encoder_table.dat"
}; 

static const tANS_table_t 
  tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]{
  #include "../../ANS_tables/tANS_z_encoder_table.dat"
}; 

int main(int argc, char const *argv[])
{
  stream<coder_interf_t> in_data;
  bool using_user_args = false;
  int user_blk_size=-1;
  int user_blk_idx=-1;
  if(argc>1) {
    using_user_args = true;
    user_blk_size = atoi(argv[1]);
  }

  if(argc>2) {
    using_user_args = true;
    user_blk_idx = atoi(argv[2]);
  }

  int num_of_errors = 0;
  for (int blk_idx = 0; blk_idx < NUM_OF_BLCKS; ++blk_idx){
	  std::vector<coder_interf_t> input_vector;

    // ************
    // Generate input 
    // ************
    #ifdef USE_EXTERNAL_INPUT_VECTOR 
      cout<<"Processing external vector "<<endl;
      int block_size = EXT_VECTOR_SIZE;
      for (int i = 0; i < block_size; ++i){
        coder_interf_t in_elem = ext_input_vector[i];
        in_data.write(in_elem);
        input_vector.push_back(in_elem);
      }

    #else
      ap_uint<REM_REDUCT_SIZE> blk_remainder_reduct = blk_idx <=6? 0: blk_idx-6 ;
      if(blk_remainder_reduct >7) blk_remainder_reduct =7;
      const int blk_remainder_bits = EE_REMAINDER_SIZE - blk_remainder_reduct;
      const int Z_mask = (1<<blk_remainder_bits)-1;

      if(using_user_args && user_blk_idx!= -1) {
        blk_idx = user_blk_idx;
      }
      cout<<"Processing block "<<blk_idx<<" | Z_mask: "<<hex<<Z_mask<<endl;
      int block_size = TEST_BUFFER_SIZE + 20- 20*int(blk_idx/2);

      if(using_user_args && user_blk_size!= -1) {
        block_size = user_blk_size;
      }
      for (int i = 0; i < block_size; ++i){
        symb_data_t symb_data;
        ap_uint<1> last_symbol = (i == block_size-1)? 1:0 ;
        ap_uint<REM_REDUCT_SIZE> in_remainder_reduct = blk_remainder_reduct;

        int val = i+TEST_BUFFER_SIZE*blk_idx;
        ap_uint<Z_SIZE> z = blk_idx <= 3? val&0x1:(blk_idx <= 5? val & 0xF : val & 0x7F) ;
        z &= ap_uint<Z_SIZE>(Z_mask);

        ap_uint<Y_SIZE> y = val & 0x80?1:0 ; 
        ap_uint<THETA_SIZE> theta_id = i >= NUM_ANS_THETA_MODES?NUM_ANS_THETA_MODES-1: i ;
        ap_uint<P_SIZE> p_id = blk_idx/2 ;
        symb_data = (z,y,theta_id,p_id);
        coder_interf_t in_inf_data = (last_symbol,in_remainder_reduct,symb_data);
        in_data <<in_inf_data;
        // in_data.write(bits_to_intf(symb_data ,symb_ctrl));
        input_vector.push_back(in_inf_data);
      }
    #endif

    // ************
    // Run DUT :
    // ************
    stream<TSG_out_intf> axis_byte_blocks; 
    stream<tsg_blk_metadata> out_blk_metadata;
    int call_cnt = 0;
    while(!in_data.empty()){
    	TSG_coder(in_data,axis_byte_blocks,out_blk_metadata
        #if defined(EXTERNAL_ANS_ROM) && !defined(USE_TSG_INTERNAL_ROM)
        ,tANS_y_encode_table, tANS_z_encode_table
        #endif
        );
    	call_cnt++;
    }


    // ************
    // Check output
    // ************
    const int golden_call_cnt = ceil(float(block_size)/BUFFER_SIZE);
    ASSERT(call_cnt, ==, golden_call_cnt);
    ASSERT(axis_byte_blocks.empty(),==,false);
    ASSERT(out_blk_metadata.empty(),==,false);


    stream<byte_block<OUT_WORD_BYTES>> byte_blocks;
    axis2byteblock<OUT_WORD_BYTES>(axis_byte_blocks,byte_blocks);
    //byte packing into wider words
    // this is an optional stage that might allow higher pl to DDR Bandwidth
    stream<byte_block<OUT_DMA_BYTES>> packed_byte_block;
    pack_out_bytes_sw_little_endian(byte_blocks,packed_byte_block);

    ASSERT(packed_byte_block.empty(),==,false);

    ap_uint<OUTPUT_STACK_BYTES_SIZE> hw_last_byte_idx_element;
    ap_uint<1> hw_last_block;
    (hw_last_byte_idx_element,hw_last_block)=out_blk_metadata.read();
    
    // Replicate AXIS to DRAM:  convert stream in array 
    unsigned mem_pointer = 0,blk_byte_cnt=0;

    int byte_counter = 0; // counter to check last_byte_idx signal
    uint8_t* block_binary = new uint8_t[packed_byte_block.size()*OUT_DMA_BYTES+4]; 
    while(! packed_byte_block.empty()) {
      byte_block<OUT_DMA_BYTES> out_byte_block = packed_byte_block.read();
      byte_counter += out_byte_block.num_of_bytes();

      //write to memory
      for(unsigned j = 0; j < out_byte_block.num_of_bytes(); ++j) {
        block_binary[mem_pointer] = out_byte_block.data((j+1)*8-1,j*8);
		#ifdef DEBUG
        printf("%d: %02X\n", mem_pointer, block_binary[mem_pointer]);
		#endif
        mem_pointer++;
        blk_byte_cnt++;
      }

      // test last and num_of_bytes info
      if(blk_byte_cnt-1 == hw_last_byte_idx_element) {
        ASSERT(out_byte_block.is_last(),==,true," | mem_pointer: "
                                <<mem_pointer<< " | blk_byte_cnt: blk_byte_cnt");
        if(!packed_byte_block.empty()) {
          ASSERT(hw_last_block,==,0);
          (hw_last_byte_idx_element,hw_last_block)=out_blk_metadata.read();
        }else{
          ASSERT(hw_last_block,==,1);
        }
        blk_byte_cnt =0;
      #ifdef DEBUG
        cout<<"End of code block"<<endl;
      #endif
      }else{
        ASSERT(out_byte_block.is_last(),==,false," | mem_pointer: "
                                <<mem_pointer<< " | blk_byte_cnt: blk_byte_cnt"); 
        ASSERT(out_byte_block.num_of_bytes(),==,OUT_DMA_BYTES," | mem_pointer: "
                                <<mem_pointer<< " | blk_byte_cnt: blk_byte_cnt");
      }

    }
    // used to check words read at the end of decoding process
    const int last_mem_pointer = mem_pointer-1; 

  

    // Decode and check
    Binary_Decoder bin_decoder(block_binary,block_size);
    int i = 0;
    for (auto elem_it : input_vector){
      // get golden values
      symb_data_t golden_data;
      symb_ctrl_t golden_ctrl;
      intf_to_bits(elem_it,golden_data,golden_ctrl);

      ap_uint<1> old_last_symbol ;
      ap_uint<REM_REDUCT_SIZE> golden_remainder_reduct;
      (old_last_symbol,golden_remainder_reduct)=golden_ctrl;
      const int remainder_bits = EE_REMAINDER_SIZE - golden_remainder_reduct;

      ap_uint<Z_SIZE> golden_z ;
      ap_uint<Y_SIZE> golden_y ;
      ap_uint<THETA_SIZE> golden_theta_id ;
      ap_uint<P_SIZE> golden_p_id ;
      (golden_z,golden_y,golden_theta_id,golden_p_id) = golden_data;
      
      //decode 
      int deco_z, deco_y;
      bin_decoder.retrive_TSG_symbol(golden_theta_id, 
                  golden_p_id, remainder_bits, deco_z, deco_y);

      //check
      ASSERT(deco_z,== ,golden_z,"Blk: "<<blk_idx<<" | i:"<<i);
      ASSERT(deco_y,== ,golden_y,"Blk: "<<blk_idx<<" | i:"<<i);
      i++;

    }

    // check all memory words have been used 
    // ASSERT(bin_decoder.get_current_byte_pointer(),==,last_mem_pointer);

    // Assert all hw streams have been consumed
    ASSERT(in_data.size(),==,0," Check that TSG consumed all input elements")
    ASSERT(byte_blocks.size(),==,0,
      " Check that the testbench has read all outputs generated by the hw")
    ASSERT(out_blk_metadata.size(),==,0,
      " Check that the testbench has read all outputs generated by the hw")

    cout<<"  | SUCCESS"<<endl;
    //clean up 
    delete[] block_binary;

    if(using_user_args) {
      break;
    }
  }

  return num_of_errors;
}
