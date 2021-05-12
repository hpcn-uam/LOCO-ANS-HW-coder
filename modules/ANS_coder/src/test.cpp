
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include "ANS_coder.hpp"
#include "../../coder_config.hpp"
#include "../../input_buffers/src/input_buffers.hpp"
#include "../../subsym_gen/src/subsym_gen.hpp"
#include <vector>
#include <list>

using namespace std;
using namespace hls;
#define NUM_OF_BLCKS (4)

#define TEST_BUFFER_SIZE 32

unsigned int get_bit( std::list<bit_blocks> &binary_list){
  auto block = binary_list.front();

  while(block.bits == 0) {
    binary_list.pop_front();
    block = binary_list.front();
  }
  
  unsigned int bit = block.data & (1 << (block.bits-1));
  bit = bit != 0? 1 : 0;
  block.bits--;
  binary_list.pop_front();
  binary_list.push_front(block);
  return bit;
}

struct tANS_dtable_t {
  uint symbol;
  uint prev_state;
} ;

static const tANS_dtable_t tANS_z_decode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES]{
 #include "ANS_tables/tANS_z_decoder_table.dat"
}; 

static const tANS_dtable_t tANS_y_decode_table[NUM_ANS_P_MODES][NUM_ANS_STATES]{
  #include "ANS_tables/tANS_y_decoder_table.dat"
}; 



const uint tANS_STATE_MASK = (0xFFFF >> (16-NUM_ANS_BITS));

uint tANS_decoder(tANS_dtable_t ANS_table_entry, uint &ANS_decoder_state,
  std::list<bit_blocks> &binary_list){

    assert((ANS_decoder_state >= ANS_I_RANGE_START));

    auto symbol = ANS_table_entry.symbol;
    assert((symbol >= 0));
    ANS_decoder_state = ANS_table_entry.prev_state;

    while(ANS_decoder_state < ANS_I_RANGE_START) {
      ANS_decoder_state *=2; // OPT: left shift
      ANS_decoder_state |=  get_bit(binary_list);
    }

    return symbol;
  }


int main(int argc, char const *argv[])
{
  stream<coder_interf_t> in_data;
  

  int num_of_errors = 0;
  for (int blk_idx = 0; blk_idx < NUM_OF_BLCKS; ++blk_idx){
	std::vector<coder_interf_t> input_vector;
    cout<<"Processing block "<<blk_idx<<endl;
    int block_size = TEST_BUFFER_SIZE - int(blk_idx/2);
    for (int i = 0; i < block_size; ++i){
      symb_data_t symb_data;
      symb_ctrl_t symb_ctrl = (i == block_size-1)? 1:0 ;

      int val = i+TEST_BUFFER_SIZE*blk_idx;
      ap_uint<Z_SIZE> z = blk_idx <= 1? val & 0xF : val & 0x7F ;
      ap_uint<Y_SIZE> y = val & 0x80?1:0 ; 
      ap_uint<THETA_SIZE> theta_id = i >= NUM_ANS_THETA_MODES?NUM_ANS_THETA_MODES-1: i ;
      ap_uint<P_SIZE> p_id = blk_idx/2 ;
      symb_data = (z,y,theta_id,p_id);
      in_data.write(bits_to_intf(symb_data ,symb_ctrl));
      input_vector.push_back(bits_to_intf(symb_data ,symb_ctrl));
    }

    stream<coder_interf_t> inverted_data;
    input_buffers(in_data, inverted_data);

    stream<subsymb_t> symbol_stream;
    stream<bit_blocks> bit_block_stream;
    while (! inverted_data.empty()){
      sub_symbol_gen(inverted_data,symbol_stream);
    }

    #if LOOP_IMPL
      code_symbols_loop(symbol_stream,bit_block_stream);
    #elif SPLITED_FREE_KERNELS
      stream<bit_blocks> out_bit_stream;
      stream<bit_blocks> last_state_stream;
      while (! symbol_stream.empty()){
          code_symbols(symbol_stream,out_bit_stream,last_state_stream);
      }

      while ((! out_bit_stream.empty()) || (! last_state_stream.empty()) ){
        ANS_output(out_bit_stream,last_state_stream,bit_block_stream);
      }

    #else
      while (! symbol_stream.empty()){
    	 ANS_coder(symbol_stream,bit_block_stream);
      }
    #endif

    //invert stream as ANS acts as a LIFO
    stream<bit_blocks> inverted_bit_blocks;
    {
      std::vector<bit_blocks> aux_vector;
      while(!bit_block_stream.empty()) {
        bit_blocks out_bit_block = bit_block_stream.read();
        aux_vector.push_back(out_bit_block);
      }
      while (!aux_vector.empty()){
        bit_blocks out_bit_block = aux_vector.back();
        aux_vector.pop_back();
        inverted_bit_blocks << out_bit_block;
      }

    }

    int i = 0;

    // get last ANS state
    bit_blocks last_ANS_state = inverted_bit_blocks.read();

    uint ANS_state = last_ANS_state.data;
    assert(last_ANS_state.bits == NUM_ANS_BITS+1);
    uint subsymbol;
    tANS_dtable_t dtable_entry;
    std::list<bit_blocks> binary_list;

    while(! inverted_bit_blocks.empty()) {
      bit_blocks out_bit_block = inverted_bit_blocks.read();
      binary_list.push_back(out_bit_block);
    }
    for (auto elem_it = input_vector.begin(); elem_it != input_vector.end(); ++elem_it){
      symb_data_t golden_data;
      symb_ctrl_t golden_ctrl;
      intf_to_bits(*elem_it,golden_data,golden_ctrl);
      
      // bit_blocks out_bit_block;

      ap_uint<Z_SIZE> golden_z ;
      ap_uint<Y_SIZE> golden_y ;
      ap_uint<THETA_SIZE> golden_theta_id ;
      ap_uint<P_SIZE> golden_p_id ;
      (golden_z,golden_y,golden_theta_id,golden_p_id) = golden_data;
      
      // check z



      // read first symbol using 
      // out_bit_block = inverted_bit_blocks.read();
      // binary_list.push_back(out_bit_block);
      uint tANS_table_idx = ANS_state & tANS_STATE_MASK;
      dtable_entry = tANS_z_decode_table[golden_theta_id][tANS_table_idx];
      subsymbol = tANS_decoder(dtable_entry, ANS_state,binary_list);

      auto ans_symb = subsymbol;
      const auto encoder_cardinality = tANS_cardinality_table[golden_theta_id];
      int module = ans_symb;
      int it = 1;
      while(ans_symb == encoder_cardinality){

        it++;
        
        /*if(it >= EE_MAX_ITERATIONS) {
          module = retrive_bits(escape_bits);
          break;
        }*/

        // out_bit_block = inverted_bit_blocks.read();
        // binary_list.push_back(out_bit_block);
        tANS_table_idx = ANS_state & tANS_STATE_MASK;
        dtable_entry = tANS_z_decode_table[golden_theta_id][tANS_table_idx];
        subsymbol = tANS_decoder(dtable_entry, ANS_state,binary_list);
        assert(subsymbol <= encoder_cardinality);
        ans_symb = subsymbol;
        module += ans_symb;
      }

      if(module != golden_z){
    	  cout<<"blk: "<<blk_idx<<" | i:"<<i<<" | golden_z: "<<golden_z<<
    			  " | golden_theta_id: "<<golden_theta_id <<
				  "| module: "<< module<<endl;
      }
      assert(module == golden_z);


      // check y
      // out_bit_block = inverted_bit_blocks.read();
      // binary_list.push_back(out_bit_block);
      tANS_table_idx = ANS_state & tANS_STATE_MASK;
      dtable_entry = tANS_y_decode_table[golden_p_id][tANS_table_idx];
      subsymbol = tANS_decoder(dtable_entry, ANS_state,binary_list);
      assert(subsymbol <= 1);
      assert(subsymbol == golden_y);

      i++;

    }

    assert(ANS_state == (1<<NUM_ANS_BITS));
    cout<<"  | SUCCESS"<<endl;
  }

  return num_of_errors;
}
