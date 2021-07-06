

#ifndef TSG_TEST_HPP
#define TSG_TEST_HPP

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
#include <ap_axi_sdata.h>

#define likely(x) __builtin_expect ((x), 1)
#define unlikely(x) __builtin_expect ((x), 0)

template<int DBYTES>
void axis2byteblock(stream<axis<ap_uint<DBYTES*8>,0,0,0 >> &in ,
            stream<byte_block<DBYTES>> &out){
  while(!in.empty()) {
    auto in_elem = in.read();
    byte_block<DBYTES> out_elem;
    out_elem.data = in_elem.data;
    out_elem.set_last(in_elem.last==1?true:false);
    int nbytes = log2(int(in_elem.keep)+1);
    out_elem.set_num_of_bytes(nbytes);
    out <<out_elem;
  }

}

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

template< unsigned STACK_SIZE,unsigned NUM_OUT_OF_BYTES>
void output_stack_sw(
  stream<byte_block<NUM_OUT_OF_BYTES> > &in, 
  stream<byte_block<NUM_OUT_OF_BYTES> > &out,
  ap_uint<1> &golden_stack_overflow){
  std::vector<byte_block<NUM_OUT_OF_BYTES>> aux_vector;
  byte_block<NUM_OUT_OF_BYTES> in_byte_block;
  bool first_read = true;
  do{
    in_byte_block = in.read();
    auto out_byte_block = in_byte_block;
    out_byte_block.set_last(first_read );
    aux_vector.push_back(out_byte_block);
    first_read = false;
  }while(!in_byte_block.is_last());

  while (!aux_vector.empty()){
    byte_block<NUM_OUT_OF_BYTES> out_byte_block = aux_vector.back();
    aux_vector.pop_back();
    out << out_byte_block;
  }

  golden_stack_overflow = out.size()> STACK_SIZE? 1:0;

}

template<unsigned IB,unsigned OB> // IB: in word bytes | OB: out word bytes
void pack_out_bytes_sw_little_endian(
  stream<byte_block<IB>> &in_bytes,
  stream<byte_block<OB>> &out_bitstream){

  //state variables
  static list<ap_uint<8>> byte_buffer;
  while(!in_bytes.empty()) {
    byte_block<IB> in_block;
    in_bytes >> in_block;
    ASSERT(in_block.num_of_bytes(),<=,IB)
    for(unsigned i = 0; i < in_block.num_of_bytes() ; ++i) {
      byte_buffer.push_back(in_block.data((i+1)*8-1,i*8) );
    }

    if(in_block.is_last()) {
      int byte_ptr = 0;
      byte_block<OB> out_block;
      out_block.data = 0 ;
      out_block.set_num_of_bytes(0);
      out_block.set_last(false);
      while(!byte_buffer.empty()) {
        decltype(out_block.data) new_byte =  byte_buffer.front();
        byte_buffer.pop_front();
        out_block.data |=  new_byte << (out_block.num_of_bytes()*8);
        out_block.increment_num_of_bytes();
        // out_block.data <<=8;
        // out_block.data |= decltype(out_block.data)(new_byte);
        // out_block.data = (out_block.data(23,0),new_byte );
        out_block.set_last(byte_buffer.empty() );
        if(out_block.num_of_bytes()==OB || out_block.is_last() ) {
          out_bitstream << out_block; 
          //reset block
          out_block.data = 0 ;
          out_block.set_num_of_bytes(0);
          out_block.set_last(false);
        }

      }
    }
  }

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
#if 1

class Binary_Decoder
{
  static constexpr int ANS_SYMBOL = 0;
  static constexpr int ANS_PREV_STATE = 1;
  static constexpr int tANS_STATE_SIZE = NUM_ANS_BITS ; 

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
       #include "../ANS_tables/tANS_z_decoder_table.dat"
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
        #include "../ANS_tables/tANS_y_decoder_table.dat"
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

#else

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
#endif




#endif // TSG_TEST_HPP

