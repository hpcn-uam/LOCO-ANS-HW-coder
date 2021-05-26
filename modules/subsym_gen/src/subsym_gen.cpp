
#include "subsym_gen.hpp"
#include "../../coder_config.hpp"




/*void geometric_coder(
  stream<ap_uint<SYMB_DATA_SIZE+2>  > &in_symb, 
  stream<subsymb_t> &out  
  ){
  // #pragma HLS PIPELINE enable_flush
  
  symb_data_t symb_data;
  ap_uint<1> end_of_block;
  ap_uint<1> is_first_px;
  (symb_data,end_of_block,is_first_px) = in_symb.read();

  subsymb_t subsymb;
  if(end_of_block == 1 && is_first_px == 1) { //process first pixel
    subsymb.subsymb = symb_data; //implicit assign of lower bits
    subsymb.type = SUBSYMB_BYPASS;
    subsymb.info = INPUT_BPP;
    subsymb.end_of_block = 1;
    out << subsymb;
  }else{
    predict_symbol_t pred_symbol(symb_data);
    
    // Bernoulli coder symb gen
    subsymb.end_of_block = 0;
    subsymb.subsymb = pred_symbol.y; //implicit assign of lower bits
    subsymb.info = pred_symbol.p_id; 
    subsymb.type = SUBSYMB_Y;
    out << subsymb;


    // Geometric coder symb gen

    // Escape symbol logic
      // subsymb.info = Z_SIZE; if escape symbol
    // subsymb.type = SUBSYMB_BYPASS;
    
    // const auto max_allowed_module = max_module_per_cardinality_table[symbol.theta_id ];
    const auto encoder_cardinality = tANS_cardinality_table[pred_symbol.theta_id ];
    uint module_reminder = pred_symbol.z;
    // auto current_ANS_table = tANS_encode_table[symbol.theta_id];

    #ifndef __SYNTHESIS__
      assert(encoder_cardinality>0);
    #endif
    uint ans_symb = module_reminder & (encoder_cardinality-1);
    // // int ans_symb = module_reminder % encoder_cardinality;
    // //iteration limitation
    // if (unlikely(symbol.z >= max_allowed_module)){ 
    //   // exceeds max iterations
    //   uint enc_bits = EE_REMAINDER_SIZE - symbol.remainder_reduct_bits;
    //   push_bits_to_binary_stack( symbol.z ,enc_bits);
    
    //   //use geometric_coder to code escape symbol
    //   module_reminder = max_allowed_module; 
    //   ans_symb = encoder_cardinality;
    // }

    // Geometric coder 
    Geometric_subsym_gen_loop: do {
      #pragma HLS PIPELINE rewind
      // tANS_encode(current_ANS_table, ans_symb);
      #ifndef __SYNTHESIS__
        assert(module_reminder>=0);
      #endif

      subsymb.subsymb = ans_symb; 
      subsymb.info = pred_symbol.theta_id; 
      subsymb.type = SUBSYMB_Z;
      subsymb.end_of_block = (pred_symbol.z==0 && end_of_block == 1)? 1:0;
      out << subsymb;

      module_reminder -= ans_symb;
      ans_symb = encoder_cardinality;
    }while(module_reminder != 0);



  }
}*/

void split_stream(
  stream<coder_interf_t> &in,
  stream<ap_uint <Y_SIZE+P_SIZE> > &y_stream,
  stream<ap_uint <Z_SIZE+THETA_SIZE+1> > &z_stream){
  // #pragma HLS INTERFACE ap_ctrl_none port=return
  // #pragma HLS PIPELINE
  #pragma HLS PIPELINE style=frp
  symb_data_t symb_data;
  symb_ctrl_t symb_ctrl;
  ap_uint<Z_SIZE> z;
  ap_uint<Y_SIZE> y;
  ap_uint<THETA_SIZE> theta_id;
  ap_uint<P_SIZE> p_id;

  //  intf_to_bits(in.read(), symb_data,symb_ctrl);
  (symb_ctrl,symb_data) = in.read();

  (z,y,theta_id,p_id) = symb_data;
  ap_uint <1> end_of_block = symb_ctrl(0,0);

  z_stream << (z,theta_id,end_of_block);
  y_stream << (y,p_id); // possibly a non-blocking write
  }

/*void serialize_symbols_with_z_decompose(
  stream<ap_uint <Y_SIZE+P_SIZE> > &y_stream,
  stream<ap_uint <Z_SIZE+THETA_SIZE+1> > &z_stream,
  stream<subsymb_t> &symbol_stream){
  // #pragma HLS PIPELINE style=frp

  subsymb_t y_subsymb,z_subsymb;
  ap_uint<Y_SIZE> y;
  ap_uint<P_SIZE> p_id;

  ap_uint<Z_SIZE> z;
  ap_uint<THETA_SIZE> theta_id;
  ap_uint<1> end_of_block;

  (y,p_id) = y_stream.read();
  (z,theta_id,end_of_block)  = z_stream.read();

  y_subsymb.end_of_block = 0;
  y_subsymb.subsymb = y; //implicit assign of lower bits
  y_subsymb.info = p_id; 
  y_subsymb.type = SUBSYMB_Y;
  symbol_stream << y_subsymb;


  const uint encoder_cardinality = 16;
  // const auto encoder_cardinality = tANS_cardinality_table[pred_symbol.theta_id ];
  uint module_reminder = z;
  // auto current_ANS_table = tANS_encode_table[symbol.theta_id];

  #ifndef __SYNTHESIS__
    assert(encoder_cardinality>0);
  #endif
  uint ans_symb = module_reminder & (encoder_cardinality-1);
  // // int ans_symb = module_reminder % encoder_cardinality;
  // //iteration limitation
  // if (unlikely(symbol.z >= max_allowed_module)){ 
  //   // exceeds max iterations
  //   uint enc_bits = EE_REMAINDER_SIZE - symbol.remainder_reduct_bits;
  //   push_bits_to_binary_stack( symbol.z ,enc_bits);
  
  //   //use geometric_coder to code escape symbol
  //   module_reminder = max_allowed_module; 
  //   ans_symb = encoder_cardinality;
  // }

  // Geometric coder 
  Geometric_subsym_gen_loop: do {
    #pragma HLS PIPELINE  style=flp
    // tANS_encode(current_ANS_table, ans_symb);
    #ifndef __SYNTHESIS__
      assert(module_reminder>=0);
    #endif

    z_subsymb.subsymb = ans_symb; 
    z_subsymb.info = theta_id; 
    z_subsymb.type = SUBSYMB_Z;
    z_subsymb.end_of_block = (module_reminder==0 && end_of_block == 1)? 1:0;
    symbol_stream << z_subsymb;

    module_reminder -= ans_symb;
    ans_symb = encoder_cardinality;
  }while(module_reminder != 0);

  // z_subsymb.end_of_block = end_of_block;
  // z_subsymb.subsymb = z; //implicit assign of lower bits
  // z_subsymb.info = theta_id; 
  // z_subsymb.type = SUBSYMB_Z;
  // symbol_stream << z_subsymb;

}*/

/*void z_decompose_2(
  stream<ap_uint <Z_SIZE+THETA_SIZE+1> > &z_stream,
  stream<subsymb_t> &z_decomposed
  ){
  // #pragma HLS INTERFACE ap_ctrl_none port=return

  subsymb_t z_subsymb;

  ap_uint<Z_SIZE> z;
  ap_uint<THETA_SIZE> theta_id;
  ap_uint<1> end_of_block;

  (z,theta_id,end_of_block)  = z_stream.read();

  const uint encoder_cardinality = 16;
  // const auto encoder_cardinality = tANS_cardinality_table[pred_symbol.theta_id ];
  ap_uint<Z_SIZE> module_reminder = z;
  // auto current_ANS_table = tANS_encode_table[symbol.theta_id];

  #ifndef __SYNTHESIS__
    assert(encoder_cardinality>0);
  #endif

  //TODO: OPT Z_SIZE is much bigger than actual size
  ap_uint<Z_SIZE> ans_symb = module_reminder & (encoder_cardinality-1); 
  // // int ans_symb = module_reminder % encoder_cardinality;
  // //iteration limitation
  // if (unlikely(symbol.z >= max_allowed_module)){ 
  //   // exceeds max iterations
  //   uint enc_bits = EE_REMAINDER_SIZE - symbol.remainder_reduct_bits;
  //   push_bits_to_binary_stack( symbol.z ,enc_bits);
  
  //   //use geometric_coder to code escape symbol
  //   module_reminder = max_allowed_module; 
  //   ans_symb = encoder_cardinality;
  // }

  // Geometric coder 
  z_decompose_loop: do {
    module_reminder -= ans_symb;
    #pragma HLS PIPELINE 
    // tANS_encode(current_ANS_table, ans_symb);
    #ifndef __SYNTHESIS__
      assert(module_reminder>=0);
    #endif

    z_subsymb.subsymb = ans_symb; 
    z_subsymb.info = theta_id; 
    z_subsymb.type = module_reminder==0 ?SUBSYMB_Z_LAST:SUBSYMB_Z;
    z_subsymb.end_of_block = (module_reminder==0 && end_of_block == 1)? 1:0;
    z_decomposed << z_subsymb;

    ans_symb = encoder_cardinality;
  }while(module_reminder != 0);
}*/


#define Z_META_STREAM_SIZE (CARD_BITS+ANS_SYMB_BITS+ 1+ 2*Z_SIZE+THETA_SIZE+1)
void z_decompose_pre(
  stream<ap_uint <Z_SIZE+THETA_SIZE+1> > &z_stream,
  stream<ap_uint <Z_META_STREAM_SIZE> > &z_stream_with_meta
  ){

  #pragma HLS PIPELINE style=frp

  ap_uint<Z_SIZE> z;
  ap_uint<THETA_SIZE> theta_id;
  ap_uint<1> end_of_block;
  (z,theta_id,end_of_block)  = z_stream.read();

  const ap_uint<CARD_BITS> encoder_cardinality = tANS_cardinality_table[theta_id ];
  ASSERT(encoder_cardinality>0);
  const ap_uint<Z_SIZE> max_allowed_module = max_module_per_cardinality_table[theta_id ];
  ASSERT(EE_MAX_ITERATIONS*int(encoder_cardinality) == max_allowed_module); // check no max mod overflow
  
  bool send_escape_symbol = z >= max_allowed_module;
  ap_uint<ANS_SYMB_BITS> ans_symb = send_escape_symbol ? encoder_cardinality :
                                  ap_uint<ANS_SYMB_BITS>(z & (encoder_cardinality-1)); // compute modulo op
  ap_uint<Z_SIZE> module_reminder = send_escape_symbol ? max_allowed_module: z;
  ap_uint<1> escape_symbol_flag = send_escape_symbol ? 1: 0 ;

  z_stream_with_meta << (encoder_cardinality,ans_symb,module_reminder,
                            escape_symbol_flag,z,theta_id,end_of_block);
}

void z_decompose_post(
  stream<ap_uint <Z_META_STREAM_SIZE> > &z_stream_with_meta,
  stream<subsymb_t> &z_decomposed
  ){

  #pragma HLS PIPELINE style=frp
  
  const ap_uint<LOG2_BIT_BLOCK_SIZE> remainder_bits = EE_REMAINDER_SIZE - 0; //symbol.remainder_reduct_bits;
  
  static ap_uint<Z_SIZE> module_reminder = 0;
  #pragma HLS reset variable=module_reminder
  static ap_uint<ANS_SYMB_BITS> ans_symb;
  static ap_uint<CARD_BITS> encoder_cardinality;
  static ap_uint<THETA_SIZE> theta_id;
  static ap_uint<1> end_of_block;
  static ap_uint<1> escape_symbol_flag;
  ap_uint<Z_SIZE> z;

  subsymb_t out_subsymb;


  if(module_reminder == 0) { // if module_reminder == 0 then prev. symbol is done
    (encoder_cardinality,ans_symb,module_reminder,
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


// WITH  #pragma HLS PIPELINE rewind: achieves II=1, but can stall
// with  #pragma HLS PIPELINE style=flp :the loop achieves II=1, but the function 
//     requires 2 extra cycles, I can afford just 1 extra cycle
/*void z_decompose_post_loop(
  stream<ap_uint <Z_META_STREAM_SIZE> > &z_stream_with_meta,
  stream<subsymb_t> &z_decomposed
  ){
  // #pragma HLS INTERFACE ap_ctrl_none port=return

  subsymb_t z_subsymb;

  const ap_uint<LOG2_BIT_BLOCK_SIZE> remainder_bits = EE_REMAINDER_SIZE - 0; //symbol.remainder_reduct_bits;
  ap_uint<Z_SIZE> z;
  ap_uint<THETA_SIZE> theta_id;
  ap_uint<1> end_of_block;
  const ap_uint<CARD_BITS> encoder_cardinality;
  ap_uint<ANS_SYMB_BITS> ans_symb;
  ap_uint<Z_SIZE> module_reminder;
  ap_uint<1> escape_symbol_flag;
  (encoder_cardinality,ans_symb,module_reminder,
      escape_symbol_flag,z,theta_id,end_of_block) = z_stream_with_meta.read();


  //TODO: OPT Z_SIZE is much bigger than actual size
  // // int ans_symb = module_reminder % encoder_cardinality;
  // //iteration limitation
  if (escape_symbol_flag == 1) { // send escape symbol
    z_subsymb.subsymb = z; 
    z_subsymb.info = remainder_bits; 
    z_subsymb.type = SUBSYMB_BYPASS;
    z_subsymb.end_of_block = 0;
    z_decomposed << z_subsymb;
  }

  // Geometric coder 
  z_decompose_loop: do {
    #pragma HLS PIPELINE rewind 
    // tANS_encode(current_ANS_table, ans_symb);
    #ifndef __SYNTHESIS__
      ASSERT(module_reminder>=0);
    #endif

    module_reminder -= ans_symb;
    z_subsymb.subsymb = ans_symb; 
    z_subsymb.info = theta_id; 
    z_subsymb.type = module_reminder==0 ?SUBSYMB_Z_LAST:SUBSYMB_Z;
    z_subsymb.end_of_block = (module_reminder==0 && end_of_block == 1)? 1:0;
    z_decomposed << z_subsymb;

    ans_symb = encoder_cardinality;
    // module_reminder -= encoder_cardinality;
    // module_reminder -= ans_symb;
  }while(module_reminder != 0);
}*/


void serialize_symbols(
  stream<ap_uint <Y_SIZE+P_SIZE> > &y_stream,
  stream<subsymb_t > &z_decomposed,
  stream<subsymb_t> &symbol_stream){
  // #pragma HLS INTERFACE ap_ctrl_none port=return
  
  #pragma HLS PIPELINE style=frp

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

// Next function achieves II=1
// problem: need #pragma HLS PIPELINE rewind , which is not
// compatible with enable_flush/style=flp
// This makes it stall
// Fix: #pragma HLS PIPELINE style=flp, but that doesn't 
// allow for II=1
/*void serialize_symbols_loop_rewind(
  stream<ap_uint <Y_SIZE+P_SIZE> > &y_stream,
  stream<subsymb_t > &z_decomposed,
  stream<subsymb_t> &symbol_stream){
  // #pragma HLS INTERFACE ap_ctrl_none port=return
  
  static ap_uint<1> input_select = 0; // 0 -> y | 1 -> z
  subsymb_t subsymb;

  serialize_loop: do{
    #pragma HLS PIPELINE rewind 
    // #pragma HLS PIPELINE style=flp

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
      if(subsymb.type == SUBSYMB_Z_LAST) {
        input_select =0;
      }
    }

    symbol_stream << subsymb;
  }while(subsymb.type != SUBSYMB_Z_LAST);


}*/

/*void serialize_symbols_1(
  stream<ap_uint <Y_SIZE+P_SIZE> > &y_stream,
  stream<subsymb_t > &z_decomposed,
  stream<subsymb_t> &symbol_stream){
  #pragma HLS INTERFACE ap_ctrl_none port=return
  

  subsymb_t y_subsymb,z_subsymb;
  ap_uint<Y_SIZE> y;
  ap_uint<P_SIZE> p_id;

  (y,p_id) = y_stream.read();

  y_subsymb.end_of_block = 0;
  y_subsymb.subsymb = y; //implicit assign of lower bits
  y_subsymb.info = p_id; 
  y_subsymb.type = SUBSYMB_Y;
  symbol_stream << y_subsymb;

  serialize_z_loop: do{
    #pragma HLS PIPELINE rewind 
    // #pragma HLS PIPELINEstyle=flp
    z_subsymb = z_decomposed.read();
    symbol_stream << z_subsymb;
  }while(z_subsymb.type != SUBSYMB_Z_LAST);

}*/

void sub_symbol_gen(
  stream<coder_interf_t> &in,
  stream<subsymb_t> &symbol_stream
  ){
  #if SUB_SYMBOL_GEN_TOP
    #pragma HLS INTERFACE axis register_mode=both register port=in
    #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
  #endif

  #pragma HLS DATAFLOW disable_start_propagation
  #pragma HLS INTERFACE ap_ctrl_none port=return
  
  stream<ap_uint <Y_SIZE+P_SIZE> > y_stream;
  #pragma HLS STREAM variable=y_stream depth=8
  stream<ap_uint <Z_SIZE+THETA_SIZE+1> > z_stream;
  #pragma HLS STREAM variable=z_stream depth=2
  split_stream(in,y_stream,z_stream);

  stream<ap_uint <Z_META_STREAM_SIZE> > z_stream_with_meta;
  #pragma HLS STREAM variable=z_stream_with_meta depth=2
  z_decompose_pre(z_stream,z_stream_with_meta);

  stream<subsymb_t> z_decomposed;
  #pragma HLS STREAM variable=z_decomposed depth=4
  z_decompose_post(z_stream_with_meta,z_decomposed);

  serialize_symbols(y_stream,z_decomposed,symbol_stream);

}
