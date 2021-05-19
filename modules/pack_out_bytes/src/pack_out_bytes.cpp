
#include "pack_out_bytes.hpp"


void empty_trailing_remover(
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
}

// This function packs bits into words of the size determined by the configuration
// file.
// Assumtions: 
//   * It assumes that the last byte_block is not empty (.bits == 0)
//     If the last byte_block is empty it fails to finish the stream with a 
//     last terminator. 
//     This function was designed to be used after 
//     serialize_last_state --> pack_out_bits --> output_stack. 
//     This secuence never sends the last byte_block with .byte ==0.
//     If this functionality is needed, add a function before pack_out_bytes
//     that removes the trailing empty byte_block and sets the last terminator
//     in the last non empty byte_block
//     
void pack_out_bytes(
  stream<byte_block> &in_bytes,
  stream<byte_block> &out_bytes){
  #if PACK_OUT_BYTES_TOP
    #pragma HLS INTERFACE axis register_mode=both register port=in_bytes
    #pragma HLS INTERFACE axis register_mode=both register port=out_bytes
    #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif
  
  #pragma HLS PIPELINE style=frp
  //state variables
  static ap_uint<1> send_remaining_data = 0; // if 1, send the data in the bit_buffer
  static ap_uint<LOG2_OUT_WORD_BYTES+1> byte_ptr=0;
  static ap_uint<2*decltype(byte_block::data)::width-8> byte_buffer=0;

  // START OF SW ONLY LOOP. Not needed in HW as it's a free running pipeline
  #ifndef __SYNTHESIS__
  while(!in_bytes.empty() || send_remaining_data == 1) {
  #endif
    if(send_remaining_data == 0){
      byte_block in_block;
      in_bytes >> in_block;

      decltype(byte_ptr) resulting_byte_ptr = byte_ptr+in_block.bytes;
      ASSERT(byte_ptr,<=,resulting_byte_ptr); // check no overflow
      ASSERT(resulting_byte_ptr,<=,decltype(byte_buffer)::width);

      // next conditional is equivalent to: 
      //          resulting_byte_ptr != OUTPUT_SIZE || resulting_byte_ptr != 0 ?
      // Which is equivalent to: 
      //          byte_ptr>0?, after the if(byte_ptr >= OUTPUT_SIZE) {} block
      send_remaining_data = resulting_byte_ptr(LOG2_OUT_WORD_BYTES-1,0) != 0 ? 
                                              in_block.last_block : ap_uint<1> (0);

      #if 0 // Using mask results in a simpler and faster HW, the logic to conditionally 
            // assign values to the bit buffer (code after #else) is expensive
      // in_block.data(in_block.data.length()-1,in_block.bytes) = 0; // ensure upper bits are zero
      // in_block.data &= decltype(in_block.data)((1<<in_block.bytes)-1); // ensure upper bits are zero
      in_block.data &= decltype(in_block.data)((1<<(in_block.bytes*8))-1);
                                             
      //Next line: only lower LOG2_OUTPUT_SIZE bits of byte_ptr are selected as  
      // byte_ptr shouldn't be >= OUTPUT_SIZE. This optimizes the resulting HW
      byte_buffer << (byte_ptr(LOG2_OUT_WORD_BYTES-1,0).to_int()*8); 
      byte_buffer |= decltype(byte_buffer)(in_block.data) << (byte_ptr(LOG2_OUT_WORD_BYTES-1,0).to_int()*8); 
      #else
      if(in_block.bytes != 0) {
        // byte_ptr shouldn't be >= OUTPUT_SIZE. This optimizes the HW
        byte_buffer(byte_ptr(LOG2_OUT_WORD_BYTES-1,0) + in_block.bytes -1 ,byte_ptr(LOG2_OUTPUT_SIZE-1,0)) 
                  = in_block.data; 
      }
      #endif

      byte_ptr = resulting_byte_ptr;

      if(byte_ptr >= OUT_WORD_BYTES) {
        byte_block out_byte_block;
        out_byte_block.data = out_word_t(byte_buffer);// select lower bits
        out_byte_block.bytes = OUT_WORD_BYTES;
        out_byte_block.last_block = byte_ptr == OUT_WORD_BYTES? in_block.last_block : ap_uint<1>(0);

        out_bytes << out_byte_block; 
        byte_ptr -= OUT_WORD_BYTES; // OPT: OUT_WORD_BYTES is a power of 2, then I can just truncate

        //Next line: equivalent to: byte_buffer >>=OUT_WORD_BYTES*8;
        byte_buffer = byte_buffer(decltype(byte_buffer)::width-1,OUT_WORD_BYTES*8); 

        // if out_byte_block.last_block ==1. Then, send_remaining_data, byte_ptr, byte_buffer 
        // are all 0, ready for next block (No need to reset them explicitly).
      }

      // send_remaining_data = byte_ptr != 0 ?in_block.last_block : ap_uint<1> (0); 


    }else{ // send the data in the bit_buffer
      ASSERT(byte_ptr,<,OUT_WORD_BYTES ); 
      ASSERT(byte_ptr,>,0 ); 
      byte_block out_byte_block;
      out_byte_block.data = out_word_t(byte_buffer);// select lower bits
      out_byte_block.bytes = byte_ptr;
      out_byte_block.last_block = 1;
      out_bytes << out_byte_block; 

      // reset
      send_remaining_data = 0; 
      byte_ptr = 0;
      byte_buffer=0;
    }

  // END OF SW ONLY LOOP. 
  #ifndef __SYNTHESIS__
  }
  #endif

}
