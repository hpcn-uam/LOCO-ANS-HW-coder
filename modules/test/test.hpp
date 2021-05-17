
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

#define likely(x) __builtin_expect ((x), 1)
#define unlikely(x) __builtin_expect ((x), 0)



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
  binary_list.pop_front();
  

  return block;
}

struct tANS_dtable_t {
  uint symbol;
  uint prev_state;
} ;





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




/***********************
 *Decoder side functions  
***********************/

/*
When calling any of :
- retrive_pixel
- retrive_TSG_symbol

it's assumed that a symbol was retrieved (except in retrive_TSG_symbol 
when the escape symbol is encountered ). So the falling clean up code is used 
before returning:

```
blk_rem_symbols--;
check_update_block();
```

*/

class Binary_Decoder
{

  static constexpr int ANS_SYMBOL = 0;
  static constexpr int ANS_PREV_STATE = 1;
  static constexpr int tANS_STATE_SIZE = NUM_ANS_BITS ; 

  uint remaining_symbols; // remaining symbols excluding those in the current block
  uint blk_rem_symbols;  // remaining symbols in current decoding block

  uint32_t * block_binary;
  size_t binary_file_ptr;
  int bit_ptr;
  static constexpr int bit_ptr_init = sizeof(*block_binary)*8-1;


public:
  Binary_Decoder(decltype(block_binary) _block_binary,uint total_symbs):
      block_binary(_block_binary),binary_file_ptr(0),bit_ptr(bit_ptr_init),
      is_ANS_ready(false),ANS_decoder_state(0){
    // TODO should be using the buffer size stated in the global header, not BUFFER_SIZE
    blk_rem_symbols = total_symbs < BUFFER_SIZE? total_symbs : BUFFER_SIZE;
    remaining_symbols = total_symbs - blk_rem_symbols;
  }

  ~Binary_Decoder(){
    if((remaining_symbols + blk_rem_symbols)>0) {
      std::cerr<<" Quiting with symbols in the binary"<<std::endl;
    }
  };
  
  /*unsigned int retrive_pixel(int bits){
    unsigned int symbol;
    assert(bits<=16);

    if (bits <=8){
      symbol = get_byte();
    }else{
      symbol = get_byte();
      symbol |= get_byte()<<8;
    }

    blk_rem_symbols--;
    check_update_block();
    return symbol;
  }*/

  int retrive_bits(int num_of_bits ){
    int bit_buffer = 0;
    for(int i = 0; i < num_of_bits; ++i) {
      bit_buffer *=2;
      bit_buffer |= get_bit();
    }

    // blk_rem_symbols--;
    // check_update_block();
    return bit_buffer;
    
  }

  int retrive_TSG_symbol(int theta_id, int p_id, uint escape_bits, int &z, int &y){  
    // z first and y second
    z = Geometric_decoder(theta_id,escape_bits);
    y = Bernoulli_decoder(p_id);
    blk_rem_symbols--;
    check_update_block();
    return 0;
  }

private:

  // operations on binary file
    /*inline unsigned int get_byte(){
      if(unlikely(bit_ptr != bit_ptr_init)) {
        std::cerr<<DBG_INFO<<" Warning: previous byte was being read. bit_ptr: "<<bit_ptr<<std::endl;
        binary_file_ptr ++; //go to next byte, discarding current
      }
      unsigned int symbol = block_binary[binary_file_ptr];
      binary_file_ptr ++;
      return symbol;
    }*/

    inline unsigned int get_word(){
      if(unlikely(bit_ptr != bit_ptr_init)) {
        std::cerr<<" Warning: previous byte was being read. bit_ptr: "<<bit_ptr<<std::endl;
        binary_file_ptr ++; //go to next byte, discarding current
      }
      unsigned int symbol = block_binary[binary_file_ptr];
      binary_file_ptr ++;
      return symbol;
    }

    inline unsigned int get_bit(){
      unsigned int bit = block_binary[binary_file_ptr] & (1 << bit_ptr);
      bit = bit != 0? 1 : 0;
      bit_ptr--;
      if(bit_ptr < 0) {
        bit_ptr = bit_ptr_init;
        binary_file_ptr++;
      }
      return bit;
    }



  bool is_ANS_ready;
  uint ANS_decoder_state;


  inline void check_update_block(){
    if(unlikely(blk_rem_symbols == 0)) {
      tANS_finish_block();
      blk_rem_symbols = remaining_symbols < BUFFER_SIZE? remaining_symbols : BUFFER_SIZE;
      remaining_symbols -= blk_rem_symbols;
    }
  }

  auto tANS_z_decoder(uint mode){
    static const int32_t tANS_z_decode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][2]{
       #include "../ANS_tables/tANS_z_decoder_table.dat"
      }; // [0] number of bits, [1] next state
    assert((ANS_decoder_state >= ANS_I_RANGE_START));
    uint tANS_table_idx = ANS_decoder_state & tANS_STATE_MASK;
    auto symbol = tANS_z_decode_table[mode][tANS_table_idx][ANS_SYMBOL];
    ANS_decoder_state = tANS_z_decode_table[mode][tANS_table_idx][ANS_PREV_STATE];
    assert((symbol >= 0));

    while(ANS_decoder_state < ANS_I_RANGE_START) {
      ANS_decoder_state *=2; // OPT: left shift
      ANS_decoder_state |=  get_bit();
    }

    return symbol;
  }

  auto tANS_y_decoder(uint mode){
    static const int32_t tANS_y_decode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
        #include "../ANS_tables/tANS_y_decoder_table.dat"
      }; // [0] number of bits, [1] next state

    assert((ANS_decoder_state >= ANS_I_RANGE_START));
    uint tANS_table_idx = ANS_decoder_state & tANS_STATE_MASK;
    auto symbol = tANS_y_decode_table[mode][tANS_table_idx][ANS_SYMBOL];
    ANS_decoder_state = tANS_y_decode_table[mode][tANS_table_idx][ANS_PREV_STATE];
    assert((symbol >= 0));

    while(ANS_decoder_state < ANS_I_RANGE_START) {
      ANS_decoder_state *=2; // OPT: left shift
      ANS_decoder_state |=  get_bit();
    }

    return symbol;
  }

  void init_ANS(){
    while(get_bit() == 0); // remove bit padding
    ANS_decoder_state = 1; // 1 because i have just removed the bit marker
    for(unsigned i = 0; i < tANS_STATE_SIZE; ++i) {
      ANS_decoder_state *=2;
      ANS_decoder_state |= get_bit();
    }

    is_ANS_ready = true;
  }

  void tANS_finish_block(){
    if(unlikely((ANS_decoder_state != ANS_I_RANGE_START))){
      std::cerr<<"Error: ANS decoder state("<<ANS_decoder_state 
            <<") should be zero at the end of the block"<<std::endl;
      throw 1;
    }
    ANS_decoder_state = 0;
    is_ANS_ready = false;
  }

  int Bernoulli_decoder(uint p_id){
    bool invert_y = false;
    #if !HALF_Y_CODER
    if(unlikely(p_id >= CTX_NT_HALF_IDX)) {
      invert_y = true;
      #if CTX_NT_CENTERED_QUANT
        if(p_id == CTX_NT_HALF_IDX) {
          return get_bit();
        }

        p_id = CTX_NT_QUANT_BINS - p_id;
      #else
        p_id = CTX_NT_QUANT_BINS-1 - p_id;
      #endif
    }
    #endif

    int ans_symb = tANS_y_decoder( p_id);
    int y = ans_symb;
    #if !HALF_Y_CODER
      if(unlikely(invert_y)) {
        y = y == 0?1:0;
      }
    #endif
    return y;
  }
  int Geometric_decoder(uint theta_id, uint escape_bits){
    if(unlikely(!is_ANS_ready)) {init_ANS(); }
    
    const auto encoder_cardinality = tANS_cardinality_table[theta_id ];

    // read first symbol using 
    auto ans_symb = tANS_z_decoder(theta_id);
    int module = ans_symb;

    int it = 1;
    while(ans_symb >= encoder_cardinality){
      if(it >= EE_MAX_ITERATIONS) {
        module = retrive_bits(escape_bits);
        break;
      }

      ans_symb = tANS_z_decoder(theta_id);
      module += ans_symb;
      it++;
    }
    

    return module;
  }


};
