
#include "pack_out_bytes.hpp"

template<unsigned BS>
void empty_trailing_remover(
  stream<byte_block<BS>> &in_bytes,
  stream<byte_block<BS>> &out_bytes){
  #pragma HLS PIPELINE style=frp

  // START OF SW ONLY LOOP. Not needed in HW as it's a free running pipeline
  #ifndef __SYNTHESIS__
  while(!in_bytes.empty() ) {
  #endif
    out_bytes.write(in_bytes.read()); // implementing NOP for now

  // END OF SW ONLY LOOP. 
  #ifndef __SYNTHESIS__
  }
  #endif
}



void pack_out_bytes_top(
  stream<byte_block<OUT_WORD_BYTES>> &in_bytes,
  stream<byte_block<OUT_DMA_BYTES>> &out_bytes){
  #if PACK_OUT_BYTES_TOP
    #pragma HLS INTERFACE axis register_mode=both register port=in_bytes
    #pragma HLS INTERFACE axis register_mode=both register port=out_bytes
    #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif
  #pragma HLS DATAFLOW disable_start_propagation
  pack_out_bytes(in_bytes,out_bytes);

};


/*
// bit endianness invertion of each individual byte
// That is, the byte endianness is not affected
void inversion_bit_endianness(
  stream<byte_block> &in_bytes,
  stream<byte_block> &out_bytes){
  #pragma HLS PIPELINE style=frp

  // START OF SW ONLY LOOP. Not needed in HW as it's a free running pipeline
  #ifndef __SYNTHESIS__
  while(!in_bytes.empty() ) {
  #endif
    constexpr int BYTE_BLOCK_BYTES = decltype(byte_block::data)::width/8;
    auto out_elem, in_elem = in_bytes.read();
    out_elem.last_block = in_elem.last_block;
    out_elem.bytes = in_elem.bytes;
    for(unsigned i = 0; i < BYTE_BLOCK_BYTES; ++i) {
      #pragma HLS UNROLL
      out_elem.data((i+1)*8-1,i*8) = in_elem.data(i*8,(i+1)*8-1);
    }
    out_bytes.write(out_elem); // implementing NOP for now

  // END OF SW ONLY LOOP. 
  #ifndef __SYNTHESIS__
  }
  #endif
}

// byte endianness inversion 
void inversion_byte_endianness(
  stream<byte_block> &in_bytes,
  stream<byte_block> &out_bytes){
  #pragma HLS PIPELINE style=frp

  // START OF SW ONLY LOOP. Not needed in HW as it's a free running pipeline
  #ifndef __SYNTHESIS__
  while(!in_bytes.empty() ) {
  #endif
    out_bytes.write(in_bytes.read()); // implementing NOP for now

  // END OF SW ONLY LOOP. 
  #ifndef __SYNTHESIS__
  }
  #endif
}*/