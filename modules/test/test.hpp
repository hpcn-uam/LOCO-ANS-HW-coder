
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include "../ANS_coder/src/ANS_coder.hpp"
#include "../coder_config.hpp"
#include "../input_buffers/src/input_buffers.hpp"
#include "../subsym_gen/src/subsym_gen.hpp"
#include <vector>
#include <list>

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

auto get_escape_symbol( std::list<bit_blocks> &binary_list){
  auto block = binary_list.front();

  while(block.bits == 0) {
    binary_list.pop_front();
    block = binary_list.front();
  }

  return block;
}

struct tANS_dtable_t {
  uint symbol;
  uint prev_state;
} ;

static const tANS_dtable_t tANS_z_decode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES]{
 #include "../ANS_tables/tANS_z_decoder_table.dat"
}; 

static const tANS_dtable_t tANS_y_decode_table[NUM_ANS_P_MODES][NUM_ANS_STATES]{
  #include "../ANS_tables/tANS_y_decoder_table.dat"
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