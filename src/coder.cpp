
#include "coder.hpp"
#include "coder_config.hpp"

coder_interf_t bits_to_intf(symb_data_t symb,symb_ctrl_t ctrl){
  return (ctrl,symb);
}

void intf_to_bits(coder_interf_t intf, symb_data_t &symb,symb_ctrl_t &ctrl){
  (ctrl,symb) = intf;
}

void write_block(
  stream<coder_interf_t> &in, 
  symb_data_t buff[BUFFER_SIZE],
  ap_uint<1> &is_first_block
  ){
#pragma HLS INLINE
  write_block_loop:for (int elem_ptr = 0; elem_ptr < BUFFER_SIZE; ++elem_ptr){
	#pragma HLS PIPELINE rewind
    symb_data_t symb_data;
    symb_ctrl_t symb_ctrl;
    intf_to_bits(in.read(),symb_data,symb_ctrl);

    if (elem_ptr == 0 ){
      is_first_block = symb_ctrl;
    }
    buff[elem_ptr] = symb_data;
  }
}

void read_block(
  const symb_data_t buff[BUFFER_SIZE],
  ap_uint<1> &is_first_block, 
  stream<ap_uint<SYMB_DATA_SIZE+2> > &out  
  ){

  read_block_loop:for (int elem_ptr = BUFFER_SIZE-1; elem_ptr >=0 ; --elem_ptr){
    #pragma HLS PIPELINE rewind
    coder_interf_t symbol;
    symb_data_t symb_data = buff[elem_ptr];
    // symb_ctrl = (end of block, is first pixel )
    ap_uint<1> is_first_px; 
    ap_uint<1> end_of_block; 
    if(elem_ptr == 0) {
      is_first_px = is_first_block;
      end_of_block = 1; 
    }else{
      is_first_px = 0;
      end_of_block = 0;
    }
    out << (symb_data,end_of_block,is_first_px) ;
  }
}

void sub_symbol_gen(
  stream<ap_uint<SYMB_DATA_SIZE+2>  > &in_symb, 
  stream<subsymb_t> &out  
  ){
//  #pragma HLS PIPELINE
  
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
    // static const ap_uint<5> tANS_cardinality_table[32] = { 16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16};
    // #pragma HLS BIND_STORAGE variable=tANS_cardinality_table type=rom_1p impl=lutram

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
  symb_data_t buffers[BUFFER_SIZE];
  ap_uint<1> is_first_block;
  #pragma HLS dataflow
  write_block(in, buffers, is_first_block);

  stream<ap_uint<SYMB_DATA_SIZE+2>  > inverted_blk_stream;
#pragma HLS STREAM variable=inverted_blk_stream depth=5
  read_block(buffers, is_first_block,inverted_blk_stream);
  sub_symbol_gen_loop: for(unsigned i = 0; i < BUFFER_SIZE; ++i) {
    #pragma HLS PIPELINE
    sub_symbol_gen(inverted_blk_stream, out);
  }

}
