
#include "ANS_coder.hpp"
#include "../../coder_config.hpp"

void tANS_encode(
  tANS_table_t ANS_table_entry,
  ap_uint<NUM_ANS_BITS> &ANS_encoder_state , 
  bit_blocks &out_bits){
  #pragma HLS INLINE

  out_bits.bits = ANS_table_entry.bits;
  ASSERT(out_bits.bits >= 0);
  out_bits.data = ANS_encoder_state;
  ANS_encoder_state = ANS_table_entry.state;
}


void code_symbols(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks> &bit_block_stream){
  
  static const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
    #include "ANS_tables/tANS_y_encoder_table.dat"
  }; // [0] number of bits, [1] next state  

  static const tANS_table_t tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][ANS_MAX_SRC_CARDINALITY]{
    #include "ANS_tables/tANS_z_encoder_table.dat"
  }; 

  static ap_uint<NUM_ANS_BITS> ANS_state = 0;

  #pragma HLS PIPELINE style=frp
  subsymb_t symbol = symbol_stream.read();
  bit_blocks out_bits;

  if(symbol.type == SUBSYMB_BYPASS) {
    out_bits.data = symbol.subsymb;
    out_bits.bits = symbol.info;
  }else{
    tANS_table_t ANS_table_entry;
    if(SUBSYMB_Y) {
      ANS_table_entry = tANS_y_encode_table[symbol.info][ANS_state][symbol.subsymb];
    }else{
      ANS_table_entry = tANS_z_encode_table[symbol.info][ANS_state][symbol.subsymb];
    }
    tANS_encode(ANS_table_entry,ANS_state ,out_bits);
  }

  bit_block_stream << out_bits;

  if(symbol.end_of_block) {

    bit_blocks last_state={(ap_uint<1>(1), ANS_state),NUM_ANS_BITS+1};
    bit_block_stream << last_state;
  }
}


void ANS_coder(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks> &bit_block_stream){
  #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
  #pragma HLS INTERFACE axis register_mode=both register port=bit_block_stream
  #pragma HLS INTERFACE ap_ctrl_none port=return
  
  #pragma HLS DATAFLOW

  code_symbols(symbol_stream,bit_block_stream);

  // main_loop:for(unsigned i = 0; i < BUFFER_SIZE; ++i) {
    
    
  // }

}

/*void ANS_coder(
  stream<coder_interf_t> &in,
  stream<bit_blocks> &bit_block_stream){
  #pragma HLS INTERFACE axis register_mode=both register port=bit_block_stream
  #pragma HLS INTERFACE axis register_mode=both register port=in
  #pragma HLS INTERFACE ap_ctrl_none port=return

  #pragma HLS DATAFLOW
  
  stream<ap_uint <Y_SIZE+P_SIZE> > y_stream;
  #pragma HLS STREAM variable=y_stream depth=8
  stream<ap_uint <Z_SIZE+THETA_SIZE+1> > z_stream;
  #pragma HLS STREAM variable=z_stream depth=2

  split_stream(in,y_stream,z_stream);

  stream<ap_uint <CARD_BITS+ANS_SYMB_BITS+ 2*Z_SIZE+THETA_SIZE+1> > z_stream_with_meta;
  #pragma HLS STREAM variable=z_stream_with_meta depth=2
  get_z_metadata(z_stream,z_stream_with_meta);

  stream<subsymb_t> z_decomposed;
  #pragma HLS STREAM variable=z_decomposed depth=4
  z_decompose_post(z_stream_with_meta,z_decomposed);


  stream<subsymb_t> symbol_stream;
  #pragma HLS STREAM variable=symbol_stream depth=2
  serialize_symbols(y_stream,z_decomposed,symbol_stream);

  
  code_symbols(symbol_stream,bit_block_stream);

  // main_loop:for(unsigned i = 0; i < BUFFER_SIZE; ++i) {
    

    
  // }

}*/
