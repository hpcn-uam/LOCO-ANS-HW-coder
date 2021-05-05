
#include "subsym_gen.hpp"
#include "../../coder_config.hpp"


/*void geometric_coder(
  stream<ap_uint<SYMB_DATA_SIZE+2>  > &in_symb, 
  stream<subsymb_t> &out  
  ){
  #pragma HLS PIPELINE enable_flush
  
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


void geometric_coder(
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
}

void split_stream(
  stream<ap_uint<SYMB_DATA_SIZE+2>> &in,
  stream<ap_uint <Y_SIZE+P_SIZE> > &y_stream,
  stream<ap_uint <Z_SIZE+THETA_SIZE+1> > &z_stream,
  stream<pixel_t > &px_stream
  ){
  #pragma HLS PIPELINE style=flp
  symb_data_t symb_data;
  ap_uint<1> end_of_block;
  ap_uint<1> is_first_px;
  (symb_data,end_of_block,is_first_px) = in.read();
  if(is_first_px) {
    px_stream << pixel_t(symb_data);
  }else{
    predict_symbol_t symbol(symb_data);
    y_stream << (symbol.y,symbol.p_id); // possibly a non-blocking write
    z_stream << (symbol.z,symbol.theta_id,end_of_block);
  }


  }

void serialize_symbols_with_z_decompose(
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

}

void z_decompose(
  stream<ap_uint <Z_SIZE+THETA_SIZE+1> > &z_stream,
  stream<subsymb_t> &z_decomposed
  ){

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
}

void serialize_symbols(
  stream<ap_uint <Y_SIZE+P_SIZE> > &y_stream,
  stream<subsymb_t > &z_decomposed,
  stream<subsymb_t> &symbol_stream){
  

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

}

void sub_symbol_gen(
  stream<ap_uint<SYMB_DATA_SIZE+2>> &in,
  stream<subsymb_t> &symbol_stream,
  stream<pixel_t > &px_stream
  ){
#pragma HLS STREAM variable=px_stream depth=4
  #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
  #pragma HLS INTERFACE axis register_mode=both register port=px_stream
  #pragma HLS INTERFACE axis register_mode=both register port=in
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #pragma HLS DATAFLOW
  // stream<ap_uint <Y_SIZE+P_SIZE> > y_stream;
  // stream<ap_uint <Z_SIZE+THETA_SIZE> > z_stream;
  // stream<pixel_t > px_stream;
  stream<ap_uint <Y_SIZE+P_SIZE> > y_stream;
  #pragma HLS STREAM variable=y_stream depth=4
  #pragma HLS STREAM variable=z_stream depth=4
  stream<ap_uint <Z_SIZE+THETA_SIZE+1> > z_stream;
  split_stream(in,y_stream,z_stream,px_stream);

  stream<subsymb_t> z_decomposed;
  #pragma HLS STREAM variable=z_decomposed depth=16
  z_decompose(z_stream,z_decomposed);
  serialize_symbols(y_stream,z_decomposed,symbol_stream);
  


  // main_loop:for(unsigned i = 0; i < BUFFER_SIZE; ++i) {
    

    
  // }

}
