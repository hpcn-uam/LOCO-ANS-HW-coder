
#include "subsym_gen.hpp"
#include "../../coder_config.hpp"


void split_stream(
  stream<coder_interf_t> &in,
  stream<ap_uint <Y_SIZE+P_SIZE> > &y_stream,
  stream<ap_uint <REM_REDUCT_SIZE+Z_SIZE+THETA_SIZE+1> > &z_stream){

  #if SPLIT_STREAM_TOP
    #pragma HLS INTERFACE axis register_mode=both register port=in
    #pragma HLS INTERFACE axis register_mode=both register port=y_stream
    #pragma HLS INTERFACE axis register_mode=both register port=z_stream
  #endif
  #pragma HLS INTERFACE ap_ctrl_none port=return

  #pragma HLS PIPELINE style=flp II=1

  ap_uint <1> end_of_block;
  ap_uint<REM_REDUCT_SIZE> remainder_reduct;
  ap_uint<Z_SIZE> z;
  ap_uint<Y_SIZE> y;
  ap_uint<THETA_SIZE> theta_id;
  ap_uint<P_SIZE> p_id;

  (end_of_block,remainder_reduct, z,y,theta_id,p_id) = in.read();

  z_stream << (z,theta_id,remainder_reduct,end_of_block);
  y_stream << (y,p_id); // possibly a non-blocking write
  }


#define Z_META_STREAM_SIZE (LOG2_Z_SIZE+CARD_BITS+ANS_SYMB_BITS+ 1+ 2*Z_SIZE+THETA_SIZE+1)
void z_decompose_pre(
  stream<ap_uint <REM_REDUCT_SIZE+Z_SIZE+THETA_SIZE+1> > &z_stream,
  stream<ap_uint <Z_META_STREAM_SIZE> > &z_stream_with_meta){

  #if Z_DECOMPOSE_PRE_TOP
    #pragma HLS INTERFACE axis register_mode=both register port=z_stream
    #pragma HLS INTERFACE axis register_mode=both register port=z_stream_with_meta
  #endif
  #pragma HLS INTERFACE ap_ctrl_none port=return

  #pragma HLS PIPELINE style=flp II=1

  ap_uint<Z_SIZE> z;
  ap_uint<THETA_SIZE> theta_id;
  ap_uint<REM_REDUCT_SIZE> remainder_reduct;
  ap_uint<1> end_of_block;
  (z,theta_id,remainder_reduct,end_of_block)  = z_stream.read();

  const ap_uint<CARD_BITS> encoder_cardinality = tANS_cardinality_table[theta_id ];
  ASSERT(encoder_cardinality>0);
  const ap_uint<Z_SIZE> max_allowed_module = max_module_per_cardinality_table[theta_id ];
  ASSERT(EE_MAX_ITERATIONS*int(encoder_cardinality) == max_allowed_module); // check no max mod overflow
  
  ap_uint<LOG2_Z_SIZE> remainder_bits = EE_REMAINDER_SIZE - remainder_reduct; //symbol.remainder_reduct_bits;
  bool send_escape_symbol = z >= max_allowed_module;
  ap_uint<ANS_SYMB_BITS> ans_symb = send_escape_symbol ? encoder_cardinality :
                                  ap_uint<ANS_SYMB_BITS>(z & (encoder_cardinality-1)); // compute modulo op
  ap_uint<Z_SIZE> module_reminder = send_escape_symbol ? max_allowed_module: z;
  ap_uint<1> escape_symbol_flag = send_escape_symbol ? 1: 0 ;

  z_stream_with_meta << (remainder_bits,encoder_cardinality,ans_symb,module_reminder,
                            escape_symbol_flag,z,theta_id,end_of_block);
}

void z_decompose_post(
  stream<ap_uint <Z_META_STREAM_SIZE> > &z_stream_with_meta,
  stream<subsymb_t> &z_decomposed){
  #if Z_DECOMPOSE_POST_TOP
    #pragma HLS INTERFACE axis register_mode=both register port=z_stream_with_meta
    #pragma HLS INTERFACE axis register_mode=both register port=z_decomposed
  #endif
  #pragma HLS INTERFACE ap_ctrl_none port=return

  #pragma HLS PIPELINE style=flp 
  // #pragma HLS PIPELINE style=flp II=1
  
  static ap_uint<Z_SIZE> module_reminder = 0;
  #pragma HLS reset variable=module_reminder
  static ap_uint<ANS_SYMB_BITS> ans_symb;
  static ap_uint<CARD_BITS> encoder_cardinality;
  static ap_uint<THETA_SIZE> theta_id;
  static ap_uint<1> end_of_block;
  static ap_uint<1> escape_symbol_flag;
  ap_uint<Z_SIZE> z;
  ap_uint<LOG2_Z_SIZE> remainder_bits;

  subsymb_t out_subsymb;


  if(module_reminder == 0) { // if module_reminder == 0 then prev. symbol is done
    (remainder_bits,encoder_cardinality,ans_symb,module_reminder,
      escape_symbol_flag,z,theta_id,end_of_block) = z_stream_with_meta.read();
  }

  // START OF SW ONLY LOOP. Not needed in HW as it's a free running pipeline
  #ifndef __SYNTHESIS__
  do{
  #endif

    if(escape_symbol_flag == 1) { // send escape symbol
      escape_symbol_flag =0; //Need to send just one of these

      //create out escape subsymbol
      out_subsymb.subsymb = z; 
      out_subsymb.info = remainder_bits; 
      out_subsymb.type = SUBSYMB_BYPASS;
      out_subsymb.end_of_block = 0; // never the last one
    }else{
      // Geometric coder 
      ASSERT(module_reminder >= (module_reminder-ans_symb ) ); // check no underflow
      module_reminder -= ans_symb; // update module_reminder given that ans_symb is sent
      
      //create out subsymbol
      out_subsymb.subsymb = ans_symb; 
      out_subsymb.info = theta_id; 
      out_subsymb.type = module_reminder==0 ?SUBSYMB_Z_LAST:SUBSYMB_Z;
      out_subsymb.end_of_block = (module_reminder==0 && end_of_block == 1)? 1:0;

      ans_symb = encoder_cardinality;
    }
    
    z_decomposed << out_subsymb;

  // END OF SW ONLY LOOP
  #ifndef __SYNTHESIS__
  }while(out_subsymb.type != SUBSYMB_Z_LAST ) ;
  #endif
}



void serialize_symbols(
  stream<ap_uint <Y_SIZE+P_SIZE> > &y_stream,
  stream<subsymb_t > &z_decomposed,
  stream<subsymb_t> &symbol_stream){
  // #pragma HLS INTERFACE ap_ctrl_none port=return
  #if SERIALIZE_SYMBOLS_TOP
    #pragma HLS INTERFACE axis register_mode=both register port=y_stream
    #pragma HLS INTERFACE axis register_mode=both register port=z_decomposed
    #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
  #endif
  #pragma HLS INTERFACE ap_ctrl_none port=return

  #pragma HLS PIPELINE style=flp II=1

  static ap_uint<1> input_select = 0; // 0 -> y | 1 -> z
  #pragma HLS reset variable=input_select
  subsymb_t subsymb;

  // START OF SW ONLY LOOP. Not needed in HW as it's a free running pipeline
  #ifndef __SYNTHESIS__
    while((!y_stream.empty()) || ((!z_decomposed.empty()))) {
  #endif

   if(input_select == 0) {
      input_select =1;
      ap_uint<Y_SIZE> y;
      ap_uint<P_SIZE> p_id;

      (y,p_id) = y_stream.read();
      
      subsymb.end_of_block = 0;
      subsymb.subsymb = y; //implicit assign of lower bits
      subsymb.info = p_id; 
      subsymb.type = SUBSYMB_Y;
    }else{
      subsymb = z_decomposed.read();
      input_select = subsymb.type == SUBSYMB_Z_LAST? 0:1;
      // if(subsymb.type == SUBSYMB_Z_LAST) {
      //   input_select =0;
      // }
    }

    symbol_stream << subsymb;

  // END OF SW ONLY LOOP
  #ifndef __SYNTHESIS__
    }
  #endif

}

void subsymbol_gen(
  stream<coder_interf_t> &in,
  stream<subsymb_t> &symbol_stream
  ){
  #if SUBSYMBOL_GEN_TOP
    #pragma HLS INTERFACE axis register_mode=both register port=in
    #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
  #endif
  #pragma HLS INTERFACE ap_ctrl_none port=return

  #pragma HLS DATAFLOW disable_start_propagation
  
  stream<ap_uint <Y_SIZE+P_SIZE> > y_stream;
  #pragma HLS bind_storage variable=y_stream type=FIFO impl=LUTRAM
  #pragma HLS STREAM variable=y_stream depth=32
  stream<ap_uint <REM_REDUCT_SIZE+Z_SIZE+THETA_SIZE+1> > z_stream;
  #pragma HLS STREAM variable=z_stream depth=2
  split_stream(in,y_stream,z_stream);

  stream<ap_uint <Z_META_STREAM_SIZE> > z_stream_with_meta;
  #pragma HLS STREAM variable=z_stream_with_meta depth=2
  z_decompose_pre(z_stream,z_stream_with_meta);

  stream<subsymb_t> z_decomposed;
  #pragma HLS STREAM variable=z_decomposed depth=32
  #pragma HLS bind_storage variable=z_decomposed type=FIFO impl=LUTRAM
  z_decompose_post(z_stream_with_meta,z_decomposed);

  serialize_symbols(y_stream,z_decomposed,symbol_stream);

}
