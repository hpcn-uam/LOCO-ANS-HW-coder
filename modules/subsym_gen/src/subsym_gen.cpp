
#include "subsym_gen.hpp"
#include "../../coder_config.hpp"


void sub_symbol_gen(
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
}



void coder(stream<coder_interf_t > &in, stream<subsymb_t > &out){
#pragma HLS INTERFACE axis register_mode=both register port=out
#pragma HLS INTERFACE axis register_mode=both register port=in
#pragma HLS INTERFACE ap_ctrl_none port=return
  
  #pragma DATAFLOW
  main_loop:for(unsigned i = 0; i < BUFFER_SIZE; ++i) {
    

    
  }

}
