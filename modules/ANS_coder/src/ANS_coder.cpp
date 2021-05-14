
#include "ANS_coder.hpp"
#include "../../coder_config.hpp"

template<size_t M>
void tANS_encode(
  tANS_table_t ANS_table_entry,
  ap_uint<NUM_ANS_BITS> &ANS_encoder_state , 
  bit_blocks_with_meta<M> &out_bits){
  #pragma HLS INLINE

  out_bits.bits = ANS_table_entry.bits;
  ASSERT(out_bits.bits >= 0);
  out_bits.data = ANS_encoder_state;
  
  #ifndef __SYNTHESIS__
    #ifdef DEBUG
      std::cout<<"Debug:     tANS_encode | In state :"<<ANS_encoder_state + (1<<NUM_ANS_BITS)<<
        " | out bits: "<<ANS_table_entry.bits<< 
        "| New state: "<< ANS_table_entry.state+ (1<<NUM_ANS_BITS)<<
        std::endl;
    #endif
  #endif

  ANS_encoder_state = ANS_table_entry.state;
}

/*void code_symbols_loop(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks> &bit_block_stream){
  
  #if CODE_SYMBOLS_LOOP_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
  #pragma HLS INTERFACE axis register_mode=both register port=bit_block_stream
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif
  
  static const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
    #include "../../ANS_tables/tANS_y_encoder_table.dat"
  }; // [0] number of bits, [1] next state  
  static const tANS_table_t tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][ANS_MAX_SRC_CARDINALITY]{
    #include "../../ANS_tables/tANS_z_encoder_table.dat"
  }; 

  ap_uint<NUM_ANS_BITS> ANS_state = 0;
  subsymb_t symbol;
  do {
    #pragma HLS PIPELINE style=flp
    symbol = symbol_stream.read();
    bit_blocks out_bits;
    out_bits.last_block=0;

    #ifndef __SYNTHESIS__
      #ifdef DEBUG
        std::cout<<"Debug: code_symbols | In symb type :"<<symbol.type<<
          " | subsymb: "<<symbol.subsymb<< 
          "| info: "<< symbol.info<<
          "| end_of_block: "<< symbol.end_of_block<<
          std::endl;
      #endif
    #endif

    if(symbol.type == SUBSYMB_BYPASS) {
      out_bits.data = symbol.subsymb;
      out_bits.bits = symbol.info;
    }else{
      tANS_table_t ANS_table_entry;
      if(symbol.type == SUBSYMB_Y) {
        ANS_table_entry = tANS_y_encode_table[symbol.info][ANS_state][symbol.subsymb];
      }else{
        ANS_table_entry = tANS_z_encode_table[symbol.info][ANS_state][symbol.subsymb];
      }
      tANS_encode(ANS_table_entry,ANS_state ,out_bits);
    }

    bit_block_stream << out_bits;


  } while (symbol.end_of_block == 0);


  bit_blocks last_state={(ap_uint<1>(1), ANS_state),NUM_ANS_BITS+1};
  bit_block_stream << last_state;
  // ANS_state = 0;
}
*/

/*void code_symbols(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks> &bit_block_stream){
  
  static const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
    #include "../../ANS_tables/tANS_y_encoder_table.dat"
  }; // [0] number of bits, [1] next state  

  static const tANS_table_t tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][ANS_MAX_SRC_CARDINALITY]{
    #include "../../ANS_tables/tANS_z_encoder_table.dat"
  }; 

  static ap_uint<NUM_ANS_BITS> ANS_state = 0;

  #pragma HLS PIPELINE style=frp
  subsymb_t symbol = symbol_stream.read();
  bit_blocks out_bits;

  #ifndef __SYNTHESIS__
    #ifdef DEBUG
      std::cout<<"Debug: code_symbols | In symb type :"<<symbol.type<<
        " | subsymb: "<<symbol.subsymb<< 
        "| info: "<< symbol.info<<
        "| end_of_block: "<< symbol.end_of_block<<
        std::endl;
    #endif
  #endif

  if(symbol.type == SUBSYMB_BYPASS) {
    out_bits.data = symbol.subsymb;
    out_bits.bits = symbol.info;
  }else{
    tANS_table_t ANS_table_entry;
    if(symbol.type == SUBSYMB_Y) {
      ANS_table_entry = tANS_y_encode_table[symbol.info][ANS_state][symbol.subsymb];
    }else{
      ANS_table_entry = tANS_z_encode_table[symbol.info][ANS_state][symbol.subsymb];
    }
    tANS_encode(ANS_table_entry,ANS_state ,out_bits);
  }

  bit_block_stream << out_bits;

  // if(symbol.end_of_block==1) {

  //   bit_blocks last_state={(ap_uint<1>(1), ANS_state),NUM_ANS_BITS+1};
  //   bit_block_stream << last_state;
  //   ANS_state = 0;
  // }
}*/

void code_symbols(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks_with_meta<NUM_ANS_BITS>> &out_bit_stream){

  #if CODE_SYMBOLS_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_bit_stream
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif

  // #pragma HLS PIPELINE style=flp
  #pragma HLS PIPELINE style=frp
  
  static const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
    #include "../../ANS_tables/tANS_y_encoder_table.dat"
  }; 

  static const tANS_table_t tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][ANS_MAX_SRC_CARDINALITY]{
    #include "../../ANS_tables/tANS_z_encoder_table.dat"
  }; 

  static ap_uint<NUM_ANS_BITS> ANS_state = 0;

  bit_blocks_with_meta<NUM_ANS_BITS> out_bits;

  subsymb_t symbol = symbol_stream.read();

  
  out_bits.last_block = symbol.end_of_block;
  #ifndef __SYNTHESIS__
    #ifdef DEBUG
      std::cout<<"Debug: code_symbols | In symb type :"<<symbol.type<<
        " | subsymb: "<<symbol.subsymb<< 
        "| info: "<< symbol.info<<
        "| end_of_block: "<< symbol.end_of_block<<
        std::endl;
    #endif
  #endif

  if(symbol.type == SUBSYMB_BYPASS) {
    out_bits.data = symbol.subsymb;
    out_bits.bits = symbol.info;
  }else{
    tANS_table_t ANS_table_entry;
    if(symbol.type == SUBSYMB_Y) {
      ANS_table_entry = tANS_y_encode_table[symbol.info][ANS_state][symbol.subsymb];
    }else{
      ANS_table_entry = tANS_z_encode_table[symbol.info][ANS_state][symbol.subsymb];
    }
    tANS_encode(ANS_table_entry,ANS_state ,out_bits);
  }

  out_bits.metadata = ANS_state;
  out_bit_stream << out_bits;

  
  if(symbol.end_of_block == 1) {
    ANS_state = 0;
  }
  
}


void serialize_last_state(
  stream<bit_blocks_with_meta<NUM_ANS_BITS>> &in_bit_blocks,
  stream<bit_blocks> &out_bit_blocks){

  #if ANS_OUTPUT_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=in_bit_blocks
  #pragma HLS INTERFACE axis register_mode=both register port=out_bit_blocks
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif

  #pragma HLS PIPELINE style=frp
  // 
  // #pragma HLS LATENCY max=1
  static ap_uint<1> send_output = 0;
  static ap_uint<NUM_ANS_BITS> ANS_state = 0;

  bit_blocks_with_meta<NUM_ANS_BITS> in_block;
  bit_blocks out_block;

  // START OF SW ONLY LOOP. Not needed in HW as it's a free running pipeline
  #ifndef __SYNTHESIS__
  do{
  #endif

    if(send_output == 1) {
      send_output = 0;
      out_block.data = (ap_uint<1>(1), ANS_state);
      out_block.bits = NUM_ANS_BITS+1;
      out_block.last_block = 1;
    }else{
      in_block = in_bit_blocks.read();
      send_output = in_block.last_block;
      ANS_state = in_block.metadata; // save just in case I need it in next cycle
      in_block.last_block =0 ;
      out_block = in_block;
    }

    out_bit_blocks << out_block;

  // END OF SW ONLY LOOP
  #ifndef __SYNTHESIS__
  }while(send_output == 1) ;
  #endif
}


/*void serialize_last_state(
  stream<bit_blocks_with_meta<NUM_ANS_BITS>> &in_bit_blocks,
  stream<bit_blocks> &out_bit_blocks){

  bit_blocks_with_meta<NUM_ANS_BITS> in_block;
  bit_blocks out_block;

  in_block = in_bit_blocks.read();
   ap_uint<1> send_output = in_block.last_block;
  ap_uint<NUM_ANS_BITS> ANS_state = in_block.metadata; // save just in case I need it in next cycle
  in_block.last_block =0 ;
  out_block = in_block;

  out_bit_blocks <<out_block;

  if(send_output == 1) {
    out_block.data = (ap_uint<1>(1), ANS_state);
    out_block.bits = NUM_ANS_BITS+1;
    out_block.last_block = 1;
    out_bit_blocks << out_block;
  }
}*/


/*void serialize_last_state(
  stream<bit_blocks> &out_bit_stream,
  stream<bit_blocks> &last_state_stream,
  stream<bit_blocks> &bit_block_stream){
  static ap_uint<1> send_output = 0;

  bit_blocks out_block;
  ANS_output_loop:do{
    #pragma HLS PIPELINE rewind
    if(send_output == 0) {
      out_block = out_bit_stream.read();
      send_output = out_block.last_block;
      out_block.last_block =0 ;
    }else{
      send_output = 0;
      out_block = last_state_stream.read();
    }

    bit_block_stream << out_block;
  }while(send_output==1);
}*/

/*void coder(stream<subsymb_t> &symbol_stream,
  stream<bit_blocks> &bit_block_stream){
  #pragma HLS PIPELINE 

  static ap_uint<NUM_ANS_BITS> ANS_state = 0;

  
  // code_symbols(symbol,bit_block_stream);
  bit_blocks out_bits;
  code_symbols(ANS_state,symbol,out_bits);
  serialize_last_state(ANS_state,out_bits,symbol,bit_block_stream);

}*/

/*void code_symbols(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks> &bit_block_stream){
  
  static const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
    #include "../../ANS_tables/tANS_y_encoder_table.dat"
  }; // [0] number of bits, [1] next state  

  static const tANS_table_t tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][ANS_MAX_SRC_CARDINALITY]{
    #include "../../ANS_tables/tANS_z_encoder_table.dat"
  }; 

  static ap_uint<NUM_ANS_BITS> ANS_state = 0;

  #pragma HLS PIPELINE style=frp
  subsymb_t symbol = symbol_stream.read();
  bit_blocks out_bits;

  #ifndef __SYNTHESIS__
    #ifdef DEBUG
      std::cout<<"Debug: code_symbols | In symb type :"<<symbol.type<<
        " | subsymb: "<<symbol.subsymb<< 
        "| info: "<< symbol.info<<
        "| end_of_block: "<< symbol.end_of_block<<
        std::endl;
    #endif
  #endif

  if(symbol.type == SUBSYMB_BYPASS) {
    out_bits.data = symbol.subsymb;
    out_bits.bits = symbol.info;
  }else{
    tANS_table_t ANS_table_entry;
    if(symbol.type == SUBSYMB_Y) {
      ANS_table_entry = tANS_y_encode_table[symbol.info][ANS_state][symbol.subsymb];
    }else{
      ANS_table_entry = tANS_z_encode_table[symbol.info][ANS_state][symbol.subsymb];
    }
    tANS_encode(ANS_table_entry,ANS_state ,out_bits);
  }

  bit_block_stream << out_bits;

  // if(symbol.end_of_block==1) {

  //   bit_blocks last_state={(ap_uint<1>(1), ANS_state),NUM_ANS_BITS+1};
  //   bit_block_stream << last_state;
  //   ANS_state = 0;
  // }
}*/

void push_bits_to_binary_stack( uint32_t symbol ,uint num_of_bits ){

  bit_buffer |= symbol << bit_ptr;
  bit_ptr += num_of_bits;

  assert(bit_ptr<=sizeof(bit_buffer)*8);

  while (bit_ptr >= BIT_BUFFER_SIZE){
    encoded_bit_stack[stack_ptr] =  binary_stack_t(bit_buffer);
    bit_ptr -= BIT_BUFFER_SIZE;
    bit_buffer >>=BIT_BUFFER_SIZE;
    stack_ptr-=sizeof(binary_stack_t);

    if(stack_ptr < 0) {
      std::cerr<<DBG_INFO<<"ERROR: Stack overflow. MAX_SUPPORTED_BPP ("<<MAX_SUPPORTED_BPP<<
            ") it's not enough. Can't fix this, quiting"<<std::endl;
      throw 1;
    }
  }

} 

void pack_out_bits(
  stream<bit_blocks> &bit_block_stream,
  stream<out_word_t> &out_bitstream){

  static ap_uint<LOG2_OUTPUT_SIZE+1> bit_ptr=0;
  ASSERT(OUTPUT_SIZE,>=,BIT_BLOCK_SIZE ); // previous ptr width declaration assumes this

  static ap_uint<OUTPUT_SIZE+BIT_BLOCK_SIZE> bit_buffer=0;

  bit_blocks in_block << bit_block_stream;

  bit_buffer |= symbol << bit_ptr;

  ASSERT(bit_ptr,>,bit_ptr+num_of_bits); // check no overflow
  bit_ptr += num_of_bits;

  ASSERT(bit_ptr,<,bit_buffer::width);


  if(bit_ptr >= OUTPUT_SIZE) {
    encoded_bit_stack[stack_ptr] =  binary_stack_t(bit_buffer);
    bit_ptr -= OUTPUT_SIZE;
    bit_buffer >>=OUTPUT_SIZE;
    stack_ptr-=sizeof(binary_stack_t);

    if(stack_ptr < 0) {
      std::cerr<<DBG_INFO<<"ERROR: Stack overflow. MAX_SUPPORTED_BPP ("<<MAX_SUPPORTED_BPP<<
            ") it's not enough. Can't fix this, quiting"<<std::endl;
      throw 1;
    }
  }


}

void ANS_coder(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks> &bit_block_stream){

  #ifdef ANS_CODER_TOP
    #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
    #pragma HLS INTERFACE axis register_mode=both register port=bit_block_stream
  #endif
  
  #pragma HLS DATAFLOW disable_start_propagation
  #pragma HLS INTERFACE ap_ctrl_none port=return
  
  stream<bit_blocks_with_meta<NUM_ANS_BITS>> out_bit_stream;
  #pragma HLS STREAM variable=out_bit_stream depth=8
  stream<bit_blocks> last_state_stream;
  #pragma HLS STREAM variable=last_state_stream depth=8
  // code_symbols(symbol,bit_block_stream);
  code_symbols(symbol_stream,out_bit_stream);

  serialize_last_state(out_bit_stream,bit_block_stream);

}

