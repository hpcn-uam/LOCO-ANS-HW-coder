
#include "merged_coder.hpp"
#include "../../coder_config.hpp"

/****** ***** ***** ***** 
***** SUBSYMBOL GEN
****** ***** ***** ***** */

void split_stream(
  stream<coder_interf_t> &in,
  stream<ap_uint <Y_SIZE+P_SIZE> > &y_stream,
  stream<ap_uint <Z_SIZE+THETA_SIZE+1> > &z_stream){
  // #pragma HLS INTERFACE ap_ctrl_none port=return
  #pragma HLS PIPELINE
  // #pragma HLS PIPELINE style=frp
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

 void z_decompose_pre(
    stream<ap_uint <Z_SIZE+THETA_SIZE+1> > &z_stream,
    stream<ap_uint <CARD_BITS+ANS_SYMB_BITS+ 2*Z_SIZE+THETA_SIZE+1> > &z_stream_with_meta
    ){
    // #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS PIPELINE 
    // #pragma HLS PIPELINE style=frp
    subsymb_t z_subsymb;

    ap_uint<Z_SIZE> z;
    ap_uint<THETA_SIZE> theta_id;
    ap_uint<1> end_of_block;

    (z,theta_id,end_of_block)  = z_stream.read();

    // const ap_uint<CARD_BITS> encoder_cardinality = 16;
    // static const ap_uint<CARD_BITS> tANS_cardinality_table[32] = { 1,2,4,8,1,2,4,8,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16};
    // 
    const ap_uint<CARD_BITS> encoder_cardinality = tANS_cardinality_table[theta_id ];
    ap_uint<Z_SIZE> module_reminder = z;
    // auto current_ANS_table = tANS_encode_table[symbol.theta_id];

    #ifndef __SYNTHESIS__
      assert(encoder_cardinality>0);
    #endif

    //TODO: OPT Z_SIZE is much bigger than actual size
    ap_uint<ANS_SYMB_BITS> ans_symb = module_reminder & (encoder_cardinality-1); 
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
    // 
    
    // module_reminder -= ans_symb;

    z_stream_with_meta << (encoder_cardinality,ans_symb,module_reminder,z,theta_id,end_of_block);
  }

  void z_decompose_post(
    stream<ap_uint <CARD_BITS+ANS_SYMB_BITS+ 2*Z_SIZE+THETA_SIZE+1> > &z_stream_with_meta,
    stream<subsymb_t> &z_decomposed
    ){
    // #pragma HLS INTERFACE ap_ctrl_none port=return

    subsymb_t z_subsymb;

    ap_uint<Z_SIZE> z;
    ap_uint<THETA_SIZE> theta_id;
    ap_uint<1> end_of_block;
    const ap_uint<CARD_BITS> encoder_cardinality;
    ap_uint<ANS_SYMB_BITS> ans_symb;
    ap_uint<Z_SIZE> module_reminder;
    (encoder_cardinality,ans_symb,module_reminder,z,theta_id,end_of_block) = z_stream_with_meta.read();


    //TODO: OPT Z_SIZE is much bigger than actual size
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
      #pragma HLS PIPELINE rewind
      // tANS_encode(current_ANS_table, ans_symb);
      #ifndef __SYNTHESIS__
        assert(module_reminder>=0);
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
  }

  void serialize_symbols(
    stream<ap_uint <Y_SIZE+P_SIZE> > &y_stream,
    stream<subsymb_t > &z_decomposed,
    stream<subsymb_t> &symbol_stream){
    // #pragma HLS INTERFACE ap_ctrl_none port=return
    
    static ap_uint<1> input_select = 0; // 0 -> y | 1 -> z
    subsymb_t subsymb;

    serialize_loop: do{
      #pragma HLS PIPELINE rewind 
      // #pragma HLS PIPELINEstyle=flp

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


  }


  void sub_symbol_gen(
    stream<coder_interf_t> &in,
    stream<subsymb_t> &symbol_stream
    ){
    // #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
    // #pragma HLS INTERFACE axis register_mode=both register depth=4 port=px_stream
    // #pragma HLS INTERFACE axis register_mode=both register port=in
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS DATAFLOW
    
    stream<ap_uint <Y_SIZE+P_SIZE> > y_stream;
    #pragma HLS STREAM variable=y_stream depth=8
    stream<ap_uint <Z_SIZE+THETA_SIZE+1> > z_stream;
    #pragma HLS STREAM variable=z_stream depth=2

    split_stream(in,y_stream,z_stream);

    stream<ap_uint <CARD_BITS+ANS_SYMB_BITS+ 2*Z_SIZE+THETA_SIZE+1> > z_stream_with_meta;
    #pragma HLS STREAM variable=z_stream_with_meta depth=2
    z_decompose_pre(z_stream,z_stream_with_meta);

    stream<subsymb_t> z_decomposed;
    #pragma HLS STREAM variable=z_decomposed depth=4
    z_decompose_post(z_stream_with_meta,z_decomposed);

    serialize_symbols(y_stream,z_decomposed,symbol_stream);

  }


/****** ***** ***** ***** 
***** ANS Coder
****** ***** ***** ***** */

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
    stream<bit_blocks> &out_bit_stream,
    stream<bit_blocks> &last_state_stream){

    #pragma HLS PIPELINE style=frp
    
    static const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
      #include "ANS_tables/tANS_y_encoder_table.dat"
    }; // [0] number of bits, [1] next state  

    static const tANS_table_t tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][ANS_MAX_SRC_CARDINALITY]{
      #include "ANS_tables/tANS_z_encoder_table.dat"
    }; 

    static ap_uint<NUM_ANS_BITS> ANS_state = 0;

    bit_blocks out_bits;
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


    out_bit_stream << out_bits;
    if(symbol.end_of_block == 1) {
      bit_blocks last_state_block;
      last_state_block.data = (ap_uint<1>(1), ANS_state);
      last_state_block.bits = NUM_ANS_BITS+1;
      last_state_block.last_block =1;
      last_state_stream <<last_state_block;
      ANS_state = 0;
    }
    
  }

  #ifndef __SYNTHESIS__
    void ANS_output(
      stream<bit_blocks> &out_bit_stream,
      stream<bit_blocks> &last_state_stream,
      stream<bit_blocks> &bit_block_stream){
    
      bit_blocks out_block;
      out_block = out_bit_stream.read();
      ap_uint<1> send_output = out_block.last_block;
      out_block.last_block =0 ;
      bit_block_stream << out_block;
    
      if(send_output == 1) {
        out_block = last_state_stream.read();
        bit_block_stream << out_block;
      }
    }
  #else
    void ANS_output(
      stream<bit_blocks> &out_bit_stream,
      stream<bit_blocks> &last_state_stream,
      stream<bit_blocks> &bit_block_stream){

      #pragma HLS PIPELINE style=frp
      static ap_uint<1> send_output = 0;

      bit_blocks out_block;
      if(send_output == 1) {
        send_output = 0;
        out_block = last_state_stream.read();
      }else{
        out_block = out_bit_stream.read();
        send_output = out_block.last_block;
        out_block.last_block =0 ;
      }

      bit_block_stream << out_block;

    }
  #endif



  void ANS_coder(
    stream<subsymb_t> &symbol_stream,
    stream<bit_blocks> &bit_block_stream){
    // #pragma HLS INTERFACE axis register_mode=both register port=symbol_stream
    // #pragma HLS INTERFACE axis register_mode=both register port=bit_block_stream
    
    #pragma HLS DATAFLOW disable_start_propagation
    #pragma HLS INTERFACE ap_ctrl_none port=return

    
    stream<bit_blocks> out_bit_stream;
    #pragma HLS STREAM variable=out_bit_stream depth=8
    stream<bit_blocks> last_state_stream;
    #pragma HLS STREAM variable=last_state_stream depth=8
    // code_symbols(symbol,bit_block_stream);
    code_symbols(symbol_stream,out_bit_stream,last_state_stream);

    ANS_output(out_bit_stream,last_state_stream,bit_block_stream);



    // coder(symbol_stream,bit_block_stream);


  }
 

/****** ***** ***** ***** 
***** TOP
****** ***** ***** ***** */

void merged_coder(
  stream<coder_interf_t> &in,
  stream<bit_blocks> &bit_block_stream){
  #pragma HLS INTERFACE axis register_mode=both register port=bit_block_stream
  #pragma HLS INTERFACE axis register_mode=both register port=in
  #pragma HLS INTERFACE ap_ctrl_none port=return

  #pragma HLS DATAFLOW disable_start_propagation
  
  stream<subsymb_t> symbol_stream;
  sub_symbol_gen(in,symbol_stream);

  #ifdef __SYNTHESIS__
    ANS_coder(symbol_stream,bit_block_stream);
  #else
    do
    {
      ANS_coder(symbol_stream,bit_block_stream);
    } while (!symbol_stream.empty());
  #endif

  // main_loop:for(unsigned i = 0; i < BUFFER_SIZE; ++i) {
    

    
  // }

}
