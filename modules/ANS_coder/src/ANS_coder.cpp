
#include "ANS_coder.hpp"
#include "../../coder_config.hpp"
// #include "ap_utils.h"

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

void tANS_encode(
  tANS_table_t ANS_table_entry,
  ap_uint<NUM_ANS_BITS> &ANS_encoder_state , 
  bit_blocks &out_bits){
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

void code_symbols(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks_with_meta<NUM_ANS_BITS>> &out_bit_stream){

  #if CODE_SYMBOLS_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_bit_stream
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif

  #pragma HLS PIPELINE style=flp II=1
  
  static const tANS_table_t 
    tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
    #include "../../ANS_tables/tANS_y_encoder_table.dat"
  }; 

  static const tANS_table_t 
    tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]{
    #include "../../ANS_tables/tANS_z_encoder_table.dat"
  }; 

  static ap_uint<NUM_ANS_BITS> ANS_state = INITIAL_ANS_STATE;
  #pragma HLS reset variable=ANS_state

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
    out_bits.metadata = ANS_state;
  }else{
    tANS_table_t ANS_table_entry;
    if(symbol.type == SUBSYMB_Y) {
      ANS_table_entry = tANS_y_encode_table[symbol.info][ANS_state][symbol.subsymb[0]];
    }else{
      ANS_table_entry = tANS_z_encode_table[symbol.info][ANS_state][symbol.subsymb(ANS_SUBSYMBOL_BITS-1,0)];
    }
    tANS_encode(ANS_table_entry,ANS_state ,out_bits);

    out_bits.metadata = ANS_state;

    if(symbol.end_of_block == 1) {
      ANS_state = INITIAL_ANS_STATE;
    }
  }

  out_bit_stream << out_bits;

  
  
}

void code_symbols_ext_ROM(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks> &out_bit_stream,
  const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2],
  const tANS_table_t  tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]
  ){

  #if CODE_SYMBOLS_EXT_ROM_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_bit_stream
  #pragma HLS INTERFACE mode=bram  port=tANS_y_encode_table storage_type=rom_1p
  #pragma HLS INTERFACE mode=bram  port=tANS_z_encode_table storage_type=rom_1p
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif

  #pragma HLS bind_storage variable=tANS_y_encode_table type=rom_1p latency=1
  #pragma HLS bind_storage variable=tANS_z_encode_table type=rom_1p latency=1
  #pragma HLS stable variable=tANS_y_encode_table
  #pragma HLS stable variable=tANS_z_encode_table

   #pragma HLS PIPELINE style=flp II=1
 // #pragma HLS PIPELINE style=flp
  // #pragma HLS latency min=3

  enum STATE { CODE=0,SEND_LAST_STATE  };
  static STATE coder_state=CODE;
  #pragma HLS reset variable=coder_state

  static ap_uint<NUM_ANS_BITS> ANS_state= INITIAL_ANS_STATE;
  #pragma HLS reset variable=ANS_state

  bit_blocks out_bits;
// START OF SW ONLY LOOP. Not needed in HW as it's a free running pipeline
  #ifndef __SYNTHESIS__
  do{
  #endif

    if(coder_state == SEND_LAST_STATE ) {
      #if SYMBOL_ENDIANNESS_LITTLE
      out_bits.data = ( ANS_state, ap_uint<1>(1));
      #else
      out_bits.data = (ap_uint<1>(1), ANS_state);
      #endif
      out_bits.bits = NUM_ANS_BITS+1;
      out_bits.last_block = 1;

      //Reset coder state
      ANS_state = INITIAL_ANS_STATE;
      coder_state=CODE;
    }else{

      subsymb_t symbol = symbol_stream.read();

      #ifndef __SYNTHESIS__
        #ifdef DEBUG
          std::cout<<"Debug: code_symbols | In symb type :"<<symbol.type<<
            " | subsymb: "<<symbol.subsymb<<
            "| info: "<< symbol.info<<
            "| end_of_block: "<< symbol.end_of_block<<
            std::endl;
        #endif
      #endif

      out_bits.last_block = 0;// the last block always contains the final ANS state

      if(symbol.type == SUBSYMB_BYPASS) {
        out_bits.data = symbol.subsymb;
        out_bits.bits = symbol.info;
      }else{
        tANS_table_t ANS_table_entry;
        if(symbol.type == SUBSYMB_Y) {
          ANS_table_entry = tANS_y_encode_table[symbol.info][ANS_state][symbol.subsymb[0]];
        }else{
          ANS_table_entry = tANS_z_encode_table[symbol.info][ANS_state][symbol.subsymb(ANS_SUBSYMBOL_BITS-1,0)];
        }
        tANS_encode(ANS_table_entry,ANS_state ,out_bits);
      }

      if(symbol.end_of_block == 1) {
        coder_state = SEND_LAST_STATE;
      }
    }
    

  out_bit_stream << out_bits;
    
  #ifndef __SYNTHESIS__
  }while(coder_state == SEND_LAST_STATE);
  #endif
}

void code_symbols_ext_ROM(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks_with_meta<NUM_ANS_BITS>> &out_bit_stream,
  const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2],
  const tANS_table_t  tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]
  ){

  #if CODE_SYMBOLS_EXT_ROM_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_bit_stream
  #pragma HLS INTERFACE mode=bram  port=tANS_y_encode_table storage_type=rom_1p
  #pragma HLS INTERFACE mode=bram  port=tANS_z_encode_table storage_type=rom_1p
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif

  #pragma HLS bind_storage variable=tANS_y_encode_table type=rom_1p latency=1
  #pragma HLS bind_storage variable=tANS_z_encode_table type=rom_1p latency=1

   #pragma HLS PIPELINE style=flp II=1
 // #pragma HLS PIPELINE style=flp
  // #pragma HLS latency min=3

  enum STATE { INIT=0,CODE  };
  static STATE coder_state=CODE;
  #pragma HLS reset variable=coder_state

  static ap_uint<NUM_ANS_BITS> ANS_state= INITIAL_ANS_STATE;
  #pragma HLS reset variable=ANS_state


  do{

    if(coder_state == INIT) {
        ANS_state = INITIAL_ANS_STATE;
        coder_state=CODE;
    }else{
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
          ANS_table_entry = tANS_y_encode_table[symbol.info][ANS_state][symbol.subsymb[0]];
        }else{
          ANS_table_entry = tANS_z_encode_table[symbol.info][ANS_state][symbol.subsymb(ANS_SUBSYMBOL_BITS-1,0)];
        }
        tANS_encode(ANS_table_entry,ANS_state ,out_bits);
      }
      out_bits.metadata = ANS_state;
      out_bit_stream << out_bits;

      if(symbol.end_of_block == 1) {
        coder_state = INIT;
      }
    }

  }while(coder_state == INIT);
}

void code_symbols_ext_ROM0(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks_with_meta<NUM_ANS_BITS>> &out_bit_stream,
  const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2],
  const tANS_table_t  tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]
  ){

  #if CODE_SYMBOLS_EXT_ROM_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_bit_stream
  #pragma HLS INTERFACE mode=bram  port=tANS_y_encode_table storage_type=rom_1p
  #pragma HLS INTERFACE mode=bram  port=tANS_z_encode_table storage_type=rom_1p
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif

	#pragma HLS bind_storage variable=tANS_y_encode_table type=rom_1p latency=1
	#pragma HLS bind_storage variable=tANS_z_encode_table type=rom_1p latency=1

   #pragma HLS PIPELINE style=flp II=1
 // #pragma HLS PIPELINE style=flp
  // #pragma HLS latency min=3 
  

  static ap_uint<NUM_ANS_BITS> ANS_state = INITIAL_ANS_STATE;
  #pragma HLS reset variable=ANS_state

  bit_blocks_with_meta<NUM_ANS_BITS> out_bits;

  subsymb_t symbol = symbol_stream.read();  
  // ap_wait();

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
    // out_bits.metadata = 0;
    // out_bits.metadata = ANS_state;
  }else{
    tANS_table_t ANS_table_entry;
    if(symbol.type == SUBSYMB_Y) {
      ANS_table_entry = tANS_y_encode_table[symbol.info][ANS_state][symbol.subsymb[0]];
    }else{
      ANS_table_entry = tANS_z_encode_table[symbol.info][ANS_state][symbol.subsymb(ANS_SUBSYMBOL_BITS-1,0)];
    }
    tANS_encode(ANS_table_entry,ANS_state ,out_bits);
    
    out_bits.metadata = ANS_state;

    if(symbol.end_of_block == 1) {
      ANS_state = INITIAL_ANS_STATE;
    }
  }

  out_bit_stream << out_bits;
  
}

/* Same as code_symbols but with different FSM
 it needs on cycle to perform initialization.
 However, it may increase clock rate (if code_symbols doesn't use set/reset pins 
 to initialize ANS_state )
 */
void code_symbols_init_state(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks_with_meta<NUM_ANS_BITS>> &out_bit_stream){

  #if CODE_SYMBOLS_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_bit_stream
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif

  #pragma HLS PIPELINE style=flp II=1
  
  static const tANS_table_t 
    tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
    #include "../../ANS_tables/tANS_y_encoder_table.dat"
  }; 

  static const tANS_table_t 
    tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]{
    #include "../../ANS_tables/tANS_z_encoder_table.dat"
  }; 

  static enum STATE {INIT=0,CODE } coder_state;
  #pragma HLS reset variable=coder_state

  static ap_uint<NUM_ANS_BITS> ANS_state = 0;
  // #pragma HLS reset variable=ANS_state

  if (coder_state == INIT) // init system
  {
    ANS_state = 0;
    coder_state = CODE;
  }else{

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
        ANS_table_entry = tANS_y_encode_table[symbol.info][ANS_state][symbol.subsymb(ANS_SUBSYMBOL_BITS-1,0)];
      }else{
        ANS_table_entry = tANS_z_encode_table[symbol.info][ANS_state][symbol.subsymb(ANS_SUBSYMBOL_BITS-1,0)];
      }
      tANS_encode(ANS_table_entry,ANS_state ,out_bits);

      if(symbol.end_of_block == 1) {
        coder_state = INIT;
        // ANS_state = 0;
      }
    }

    out_bits.metadata = ANS_state;
    out_bit_stream << out_bits;

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

  #pragma HLS PIPELINE style=flp II=1

  static ap_uint<1> send_output = 0;
  #pragma HLS reset variable=send_output
  static ap_uint<NUM_ANS_BITS> ANS_state = 0;

  bit_blocks_with_meta<NUM_ANS_BITS> in_block;
  bit_blocks out_block;

  // START OF SW ONLY LOOP. Not needed in HW as it's a free running pipeline
  #ifndef __SYNTHESIS__
  do{
  #endif

    if(send_output == 1) {
      send_output = 0;
      #if SYMBOL_ENDIANNESS_LITTLE
      out_block.data = ( ANS_state, ap_uint<1>(1));
      #else
      out_block.data = (ap_uint<1>(1), ANS_state);
      #endif
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

void ANS_coder_top(
  stream<subsymb_t> &symbol_stream,
  stream<byte_block<OUT_WORD_BYTES>> &byte_block_stream){
  #ifdef ANS_CODER_TOP
    #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
    #pragma HLS INTERFACE axis register_mode=both register port=byte_block_stream
  #endif

  #pragma HLS DATAFLOW disable_start_propagation
  #pragma HLS INTERFACE ap_ctrl_none port=return

  ANS_coder(symbol_stream,byte_block_stream);
}


void ANS_coder_ext_ROM_top(
  stream<subsymb_t> &symbol_stream,
  stream<byte_block<OUT_WORD_BYTES>> &byte_block_stream,
  const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2],
  const tANS_table_t  tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]){

  #ifdef ANS_CODER_EXT_ROM_TOP
    #pragma HLS INTERFACE mode=ap_memory  port=tANS_y_encode_table storage_type=rom_1p
    #pragma HLS INTERFACE mode=ap_memory  port=tANS_z_encode_table storage_type=rom_1p
    #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
    #pragma HLS INTERFACE axis register_mode=both register port=byte_block_stream
  #endif

  #pragma HLS DATAFLOW disable_start_propagation
  // #pragma HLS INTERFACE ap_ctrl_none port=return

  ANS_coder_ext_ROM(symbol_stream,byte_block_stream,tANS_y_encode_table,tANS_z_encode_table);
}

  


void ANS_coder_double_lane(
  stream<subsymb_t> &symbol_stream_0,
  stream<byte_block<OUT_WORD_BYTES>> &byte_block_stream_0,
  stream<subsymb_t> &symbol_stream_1,
  stream<byte_block<OUT_WORD_BYTES>> &byte_block_stream_1){
  #ifdef ANS_CODER_DOUBLE_LANE_TOP
    #pragma HLS DATAFLOW disable_start_propagation
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream_0
    #pragma HLS INTERFACE axis register_mode=both register port=byte_block_stream_0
    #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream_1
    #pragma HLS INTERFACE axis register_mode=both register port=byte_block_stream_1
  #endif
  static ANSCoder<OUT_WORD_BYTES> coder_0,coder_1;
  #pragma HLS reset variable=coder_0
  #pragma HLS reset variable=coder_1

  const tANS_table_t 
    tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
    #include "../../ANS_tables/tANS_y_encoder_table.dat"
  }; 

  const tANS_table_t 
    tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]{
    #include "../../ANS_tables/tANS_z_encoder_table.dat"
  }; 

  coder_0.code(symbol_stream_0,byte_block_stream_0,tANS_y_encode_table,tANS_z_encode_table);
  coder_1.code(symbol_stream_1,byte_block_stream_1,tANS_y_encode_table,tANS_z_encode_table);

}