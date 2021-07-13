
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include <vector>
#include <list>
#include "checker_config.hpp"

#define likely(x) __builtin_expect ((x), 1)
#define unlikely(x) __builtin_expect ((x), 0)


class Binary_Decoder
{
  static constexpr int ANS_SYMBOL = 0;
  static constexpr int ANS_PREV_STATE = 1;
  static constexpr int tANS_STATE_SIZE = NUM_ANS_BITS ; 
  static constexpr int tANS_STATE_MASK = (1<<tANS_STATE_SIZE)-1;

  uint remaining_symbols; // remaining symbols excluding those in the current block
  uint blk_rem_symbols;  // remaining symbols in current decoding block

  #if BIT_ENDIANNESS_LITTLE ||SYMBOL_ENDIANNESS_LITTLE
  uint32_t * block_binary;
  #else
  uint8_t * block_binary;
  #endif
  // unsigned char* block_binary;
  size_t binary_file_ptr;
  uint64_t binary_buffer;
  int bit_ptr; // number of bits already read from the binary_buffer
  static constexpr int BINARY_BLOCK_BYTES =  sizeof(*block_binary);
  static constexpr int BINARY_BLOCK_BITS =  BINARY_BLOCK_BYTES*8;
  #if BIT_ENDIANNESS_LITTLE || SYMBOL_ENDIANNESS_LITTLE
    static constexpr int bit_ptr_init = 0;
  #else
    static constexpr int bit_ptr_init = BINARY_BLOCK_BYTES*8-1;
  // #define bit_ptr_init (7)
  #endif



public:
  Binary_Decoder(void* _block_binary,uint total_symbs):
      block_binary(decltype(block_binary)( _block_binary)),
      #if BIT_ENDIANNESS_LITTLE ||SYMBOL_ENDIANNESS_LITTLE
      binary_file_ptr(2), // 2 cause binary_buffer is initialized with the first 2 elements
      #else
      binary_file_ptr(0),
      #endif 
      bit_ptr(bit_ptr_init),is_ANS_ready(false),ANS_decoder_state(0){
    // TODO should be using the buffer size stated in the global header, not BUFFER_SIZE
    blk_rem_symbols = total_symbs < BUFFER_SIZE? total_symbs : BUFFER_SIZE;
    remaining_symbols = total_symbs - blk_rem_symbols;
    binary_buffer = decltype(binary_buffer)(block_binary[1])<<BINARY_BLOCK_BITS | block_binary[0];
  }

  ~Binary_Decoder(){};
  
  auto get_current_byte_pointer(){
    // if bit_ptr ==0, consider that the next byte has not been read
    return (binary_file_ptr-2)*BINARY_BLOCK_BYTES + int((bit_ptr-1)/8);
  }

  unsigned int retrive_pixel(int bits){
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
  }

  #if SYMBOL_ENDIANNESS_LITTLE

  int retrive_bits(int num_of_bits ){
    unsigned int bits = (binary_buffer>>bit_ptr) & ((1<<num_of_bits)-1) ;
    bit_ptr+=num_of_bits;

    if(bit_ptr >= BINARY_BLOCK_BITS) {
      binary_buffer >>= BINARY_BLOCK_BITS;
      binary_buffer |= decltype(binary_buffer)(block_binary[binary_file_ptr])<<BINARY_BLOCK_BITS;
      bit_ptr -= BINARY_BLOCK_BITS;
      binary_file_ptr++;
    }

    return bits;
  }
  #else
  inline int retrive_bits(int num_of_bits ){
    int bit_buffer = 0;
    for(int i = 0; i < num_of_bits; ++i) {
      bit_buffer <<= 1;
      bit_buffer |= get_bit();
    }

    return bit_buffer;
    
  }
  #endif

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
    inline unsigned int get_byte(){
      #if BIT_ENDIANNESS_LITTLE || SYMBOL_ENDIANNESS_LITTLE
        if(unlikely((bit_ptr& 7) != 0)) {
      #else
        if(unlikely(bit_ptr != bit_ptr_init)) {
      #endif
        std::cerr<<" Warning: previous byte was being read. bit_ptr: "<<bit_ptr<<std::endl;

        //go to next byte, discarding current
        #if BIT_ENDIANNESS_LITTLE || SYMBOL_ENDIANNESS_LITTLE

          bit_ptr &= -8; // set lower bits to 0
          bit_ptr += 8;  // go to next byte
          if(bit_ptr >= BINARY_BLOCK_BITS) {
            binary_buffer >>= BINARY_BLOCK_BITS;
            binary_buffer |= decltype(binary_buffer)(block_binary[binary_file_ptr])<<BINARY_BLOCK_BITS;
            bit_ptr = bit_ptr_init;
            binary_file_ptr++;
          }
        #else
          binary_file_ptr ++; 
        #endif
      }
      unsigned int symbol;
      #if BIT_ENDIANNESS_LITTLE || SYMBOL_ENDIANNESS_LITTLE
        symbol = (binary_buffer>>bit_ptr) & 0xFF;
        bit_ptr += 8;
        if(bit_ptr >= BINARY_BLOCK_BITS) {
          binary_buffer >>= BINARY_BLOCK_BITS;
          binary_buffer |= decltype(binary_buffer)(block_binary[binary_file_ptr])<<BINARY_BLOCK_BITS;
          bit_ptr = bit_ptr_init;
          binary_file_ptr++;
        }
      #else
        symbol = block_binary[binary_file_ptr];
        binary_file_ptr ++;
      #endif
      return symbol;
    }

    inline unsigned int get_bit(){
      #if BIT_ENDIANNESS_LITTLE || SYMBOL_ENDIANNESS_LITTLE
      unsigned int bit = (binary_buffer>>bit_ptr) & 1 ;
      #else
      unsigned int bit = (block_binary[binary_file_ptr]>>bit_ptr) & 1 ;
      #endif
      //
      // unsigned int bit = block_binary[binary_file_ptr] & (1 << bit_ptr);
      // bit >>= bit_ptr;
      // 
      // bit = bit != 0? 1 : 0;

      #if BIT_ENDIANNESS_LITTLE || SYMBOL_ENDIANNESS_LITTLE
      bit_ptr++;
      #else
      bit_ptr--;
      #endif

      #if BIT_ENDIANNESS_LITTLE || SYMBOL_ENDIANNESS_LITTLE
        if(bit_ptr >= BINARY_BLOCK_BITS) {
          binary_buffer >>= BINARY_BLOCK_BITS;
          binary_buffer |= decltype(binary_buffer)(block_binary[binary_file_ptr])<<BINARY_BLOCK_BITS;
          bit_ptr = bit_ptr_init;
          binary_file_ptr++;
        }
      #else
        if(bit_ptr < 0) {
          bit_ptr = bit_ptr_init;
          binary_file_ptr++;
        }
      #endif
      return bit;
    }

    /*inline unsigned int get_bit(){
      unsigned int bit = block_binary[binary_file_ptr] & (1 << bit_ptr);
      bit = bit != 0? 1 : 0;
      bit_ptr--;
      if(bit_ptr < 0) {
        bit_ptr = bit_ptr_init;
        binary_file_ptr++;
      }
      return bit;
    }*/



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
       #include "./ANS_tables/tANS_z_decoder_table.dat"
      }; // [0] number of bits, [1] next state
    assert((ANS_decoder_state >= ANS_I_RANGE_START));
    uint tANS_table_idx = ANS_decoder_state & tANS_STATE_MASK;
    auto symbol = tANS_z_decode_table[mode][tANS_table_idx][ANS_SYMBOL];
    ANS_decoder_state = tANS_z_decode_table[mode][tANS_table_idx][ANS_PREV_STATE];
    assert((symbol >= 0));

    #if SYMBOL_ENDIANNESS_LITTLE
    // This can be done cause ANS_I_RANGE_START = 2^int
    int new_bits = 0;
    // This can be done cause ANS_I_RANGE_START = 2^int
    while(ANS_decoder_state < ANS_I_RANGE_START) {
      ANS_decoder_state <<=1; 
      new_bits++;
    }
    if(new_bits) {
      ANS_decoder_state |= retrive_bits(new_bits);
    }
    #else
    while(ANS_decoder_state < ANS_I_RANGE_START) {
      ANS_decoder_state *=2; // OPT: left shift
      ANS_decoder_state |=  get_bit();
    }
    #endif

    return symbol;
  }

  auto tANS_y_decoder(uint mode){
    static const int32_t tANS_y_decode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
        #include "./ANS_tables/tANS_y_decoder_table.dat"
      }; // [0] number of bits, [1] next state

    assert((ANS_decoder_state >= ANS_I_RANGE_START));
    uint tANS_table_idx = ANS_decoder_state & tANS_STATE_MASK;
    auto symbol = tANS_y_decode_table[mode][tANS_table_idx][ANS_SYMBOL];
    ANS_decoder_state = tANS_y_decode_table[mode][tANS_table_idx][ANS_PREV_STATE];
    assert((symbol >= 0));

    #if SYMBOL_ENDIANNESS_LITTLE
    int new_bits = 0;
    // This can be done cause ANS_I_RANGE_START = 2^int
    while(ANS_decoder_state < ANS_I_RANGE_START) {
      ANS_decoder_state <<=1; 
      new_bits++;
    }
    if(new_bits) {
      ANS_decoder_state |= retrive_bits(new_bits);
    }
    #else
    while(ANS_decoder_state < ANS_I_RANGE_START) {
      ANS_decoder_state *=2; // OPT: left shift
      ANS_decoder_state |=  get_bit();
    }
    #endif


    return symbol;
  }

  void init_ANS(){
    while(get_bit() == 0); // remove bit padding
    #if SYMBOL_ENDIANNESS_LITTLE
    ANS_decoder_state = 1<< tANS_STATE_SIZE;
    ANS_decoder_state |= retrive_bits(tANS_STATE_SIZE);
    #else
    ANS_decoder_state = 1; // 1 because i have just removed the bit marker
    for(unsigned i = 0; i < tANS_STATE_SIZE; ++i) {
      ANS_decoder_state <<=1;
      ANS_decoder_state |= get_bit();
    }
    #endif
    
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
