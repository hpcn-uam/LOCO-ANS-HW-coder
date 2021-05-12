
#include "TSG_coder.hpp"


void TSG_coder(
  stream<coder_interf_t> &in,
  stream<bit_blocks> &bit_block_stream){
  #pragma HLS INTERFACE axis register_mode=both register port=in
  #pragma HLS INTERFACE axis register_mode=both register port=bit_block_stream
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
}
