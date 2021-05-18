
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
  
  static const tANS_table_t 
    tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
    #include "../../ANS_tables/tANS_y_encoder_table.dat"
  }; 

  static const tANS_table_t 
    tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][ANS_MAX_SRC_CARDINALITY]{
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


// This function was used to verify the testbench of pack_out_bits 
// It's not optimized for performance in SW or HW, but to be simple.
void pack_out_bits_sw(
  stream<bit_blocks> &bit_block_stream,
  stream<byte_block> &out_bitstream){

  //state variables
  // static uint bit_ptr=0;
  static ap_uint<LOG2_OUTPUT_SIZE+1> bit_ptr=0;
  ASSERT(OUTPUT_SIZE,>=,BIT_BLOCK_SIZE ); // previous ptr width declaration assumes this
  // static long long unsigned bit_buffer=0;
  static ap_uint<OUTPUT_SIZE+BIT_BLOCK_SIZE> bit_buffer=0;


  while(!bit_block_stream.empty()) {
    bit_blocks in_block;
    bit_block_stream >> in_block;

    in_block.data &= decltype(in_block.data)((1<<in_block.bits)-1); // ensure upper bits are zero
    // in_block.data(in_block.data.length()-1,in_block.bits) = 0; // ensure upper bits are zero

    bit_buffer |= ap_uint<OUTPUT_SIZE+BIT_BLOCK_SIZE>(in_block.data) << bit_ptr; 

    ASSERT(bit_ptr,<=,bit_ptr+in_block.bits.to_int()); // check no overflow
    bit_ptr += in_block.bits;
    ASSERT(bit_ptr,<,OUTPUT_SIZE+BIT_BLOCK_SIZE);

    byte_block out_byte_block;
    if(bit_ptr >= OUTPUT_SIZE) {
      out_byte_block.data = out_word_t(bit_buffer);// select lower bits
      out_byte_block.bytes = OUT_WORD_BYTES;
      out_byte_block.last_block = bit_ptr == OUTPUT_SIZE? in_block.last_block : ap_uint<1> (0);

      out_bitstream << out_byte_block; 
      bit_ptr -= OUTPUT_SIZE; // OPT: OUTPUT_SIZE is a power of 2, then I can just truncate
      bit_buffer >>=OUTPUT_SIZE;
    }

    ASSERT(bit_ptr,<,OUTPUT_SIZE ); 
    if(in_block.last_block == 1 && bit_ptr >0){ // send the data in the bit_buffer
      byte_block out_byte_block;
      out_byte_block.data = out_word_t(bit_buffer);// select lower bits
      uint aux_ptr = bit_ptr+7;
      out_byte_block.bytes =(aux_ptr>>3);
      out_byte_block.last_block = 1;
      out_bitstream << out_byte_block; 

      // reset
      bit_ptr = 0;
      bit_buffer=0;
    }
  }

}

void pack_out_bits(
  stream<bit_blocks> &bit_block_stream,
  stream<byte_block> &out_bitstream){

  #pragma HLS PIPELINE style=frp
  //state variables
  static ap_uint<1> send_remaining_data = 0; // if 1, send the data in the bit_buffer
  static ap_uint<LOG2_OUTPUT_SIZE+1> bit_ptr=0;
  ASSERT(OUTPUT_SIZE,>=,BIT_BLOCK_SIZE ); // previous ptr width declaration assumes this
  static ap_uint<OUTPUT_SIZE+BIT_BLOCK_SIZE> bit_buffer=0;

  // START OF SW ONLY LOOP. Not needed in HW as it's a free running pipeline
  #ifndef __SYNTHESIS__
  while(!bit_block_stream.empty() || send_remaining_data == 1) {
  #endif
    if(send_remaining_data == 0){
      bit_blocks in_block;
      bit_block_stream >> in_block;

      decltype(bit_ptr) resulting_bit_ptr = bit_ptr+in_block.bits;
      ASSERT(bit_ptr,<=,resulting_bit_ptr); // check no overflow
      ASSERT(resulting_bit_ptr,<=,decltype(bit_buffer)::width);

      // next conditional is equivalent to: 
      //          resulting_bit_ptr != OUTPUT_SIZE || resulting_bit_ptr != 0 ?
      // Which is equivalent to: 
      //          bit_ptr>0?, after the if(bit_ptr >= OUTPUT_SIZE) {} block
      send_remaining_data = resulting_bit_ptr(LOG2_OUTPUT_SIZE-1,0) != 0 ? 
                                              in_block.last_block : ap_uint<1> (0);

      #if 1 // Using mask results in a simpler and faster HW, the logic to conditionally 
            // assign values to the bit buffer (code after #else) is expensive
      // in_block.data(in_block.data.length()-1,in_block.bits) = 0; // ensure upper bits are zero
      in_block.data &= decltype(in_block.data)((1<<in_block.bits)-1); // ensure upper bits are zero

      //Next line: only lower LOG2_OUTPUT_SIZE bits of bit_ptr are selected as  
      // bit_ptr shouldn't be >= OUTPUT_SIZE. This optimizes the resulting HW
      bit_buffer |= decltype(bit_buffer)(in_block.data) << bit_ptr(LOG2_OUTPUT_SIZE-1,0); 
      #else
      if(in_block.bits != 0) {
        // bit_ptr shouldn't be >= OUTPUT_SIZE. This optimizes the HW
        bit_buffer(bit_ptr(LOG2_OUTPUT_SIZE-1,0) + in_block.bits -1 ,bit_ptr(LOG2_OUTPUT_SIZE-1,0)) 
                  = in_block.data; 
      }
      #endif

      bit_ptr = resulting_bit_ptr;

      if(bit_ptr >= OUTPUT_SIZE) {
        byte_block out_byte_block;
        out_byte_block.data = out_word_t(bit_buffer);// select lower bits
        out_byte_block.bytes = OUT_WORD_BYTES;
        out_byte_block.last_block = bit_ptr == OUTPUT_SIZE? in_block.last_block : ap_uint<1>(0);

        out_bitstream << out_byte_block; 
        bit_ptr -= OUTPUT_SIZE; // OPT: OUTPUT_SIZE is a power of 2, then I can just truncate

        //Next line: equivalent to: bit_buffer >>=OUTPUT_SIZE;
        bit_buffer = bit_buffer(decltype(bit_buffer)::width-1,OUTPUT_SIZE); 

        // if out_byte_block.last_block ==1. Then, send_remaining_data, bit_ptr, bit_buffer 
        // are all 0, ready for next block (No need to reset them explicitly).
      }

      // send_remaining_data = bit_ptr != 0 ?in_block.last_block : ap_uint<1> (0); 


    }else{ // send the data in the bit_buffer
      ASSERT(bit_ptr,<,OUTPUT_SIZE ); 
      ASSERT(bit_ptr,>,0 ); 
      byte_block out_byte_block;
      out_byte_block.data = out_word_t(bit_buffer);// select lower bits
      ap_uint<LOG2_OUTPUT_SIZE+1> aux_ptr = bit_ptr+7;
      out_byte_block.bytes = aux_ptr(LOG2_OUT_WORD_BYTES+3,3);
      out_byte_block.last_block = 1;
      out_bitstream << out_byte_block; 

      // reset
      send_remaining_data = 0; 
      bit_ptr = 0;
      bit_buffer=0;
    }

  // END OF SW ONLY LOOP. 
  #ifndef __SYNTHESIS__
  }
  #endif

}


void ANS_coder(
  stream<subsymb_t> &symbol_stream,
  stream<byte_block> &byte_block_stream){

  #ifdef ANS_CODER_TOP
    #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
    #pragma HLS INTERFACE axis register_mode=both register port=bit_block_stream
  #endif
  
  #pragma HLS DATAFLOW disable_start_propagation
  #pragma HLS INTERFACE ap_ctrl_none port=return
  
  stream<bit_blocks_with_meta<NUM_ANS_BITS>> out_bit_stream;
  #pragma HLS STREAM variable=out_bit_stream depth=2
  code_symbols(symbol_stream,out_bit_stream);

  stream<bit_blocks> bit_block_stream;
  #pragma HLS STREAM variable=bit_block_stream depth=2
  serialize_last_state(out_bit_stream,bit_block_stream);

  pack_out_bits(bit_block_stream,byte_block_stream);
}
