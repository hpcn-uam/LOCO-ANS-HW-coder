
#ifndef PACK_OUT_BYTES_HPP
#define PACK_OUT_BYTES_HPP

#include <hls_stream.h>
#include <tgmath.h>

// #include "ap_axi_sdata.h"
#include "../../coder_config.hpp"
#include "../../ANS_coder/src/ANS_coder.hpp"

using namespace hls;

// Complementary function to byte-align a byte stream
void pack_out_bytes_top(
  stream<byte_block<OUT_WORD_BYTES>> &in_bytes,
  stream<byte_block<OUT_DMA_BYTES>> &out_bytes);


// This function packs bytes into words of the size determined by the template
// args.
// Byte endianness: Little
// Assumptions: 
//   * It assumes that the last byte_block is not empty (.bytes == 0)
//     If the last byte_block is empty it fails to finish the stream with a 
//     last terminator. 
//     This function was designed to be used after 
//     serialize_last_state --> pack_out_bits --> output_stack. 
//     This sequence never sends the last byte_block with .byte ==0.
//     If this functionality is needed, add a function before pack_out_bytes
//     that removes the trailing empty byte_block and sets the last terminator
//     in the last non empty byte_block
template<unsigned IB,unsigned OB> // IB: in word bytes | OB: out word bytes
void pack_out_bytes(
  stream<byte_block<IB>> &in_bytes,
  stream<byte_block<OB>> &out_bytes){

  ASSERT(IB,<=,OB)

  #pragma HLS PIPELINE style=frp

  constexpr unsigned BYTE_BUFFER_WIDTH = (2*OB-1)*8;
  constexpr unsigned BYTE_PTR_WIDTH = ceillog2(BYTE_BUFFER_WIDTH/8+1);
  constexpr unsigned LOG2_OUT_WORD_BYTES = ceillog2(OB);

  //state variables
  static ap_uint<1> send_remaining_data = 0; // if 1, send the data in the bit_buffer
  #pragma HLS reset variable=send_remaining_data
  static ap_uint<BYTE_BUFFER_WIDTH> byte_buffer=0;
  #pragma HLS reset variable=byte_buffer
  static ap_uint<BYTE_PTR_WIDTH> byte_ptr=0;
  #pragma HLS reset variable=byte_ptr

  // START OF SW ONLY LOOP. Not needed in HW as it's a free running pipeline
  #ifndef __SYNTHESIS__
  while(!in_bytes.empty() || send_remaining_data == 1) {
  #endif

    if(send_remaining_data == 0){
      byte_block<IB> in_block;
      in_bytes >> in_block;

      decltype(byte_ptr) resulting_byte_ptr = byte_ptr+in_block.num_of_bytes();
      ASSERT(byte_ptr,<=,resulting_byte_ptr); // check no overflow
      ASSERT(resulting_byte_ptr,<=,decltype(byte_buffer)::width);

      // next conditional is equivalent to: 
      //          resulting_byte_ptr != OUTPUT_SIZE || resulting_byte_ptr != 0 ?
      // Which is equivalent to: 
      //          byte_ptr>0?, after the if(byte_ptr >= OUTPUT_SIZE) {} block
      send_remaining_data = resulting_byte_ptr(LOG2_OUT_WORD_BYTES-1,0) != 0 ? 
                            (in_block.is_last()? ap_uint<1>(1):ap_uint<1>(0)) : ap_uint<1>(0);

      #if 1 // Using a mask results in a simpler and faster HW, the logic to conditionally 
            // assign values to the bit buffer (code after #else) is expensive
        // in_block.data(in_block.data.length()-1,in_block.num_of_bytes()) = 0; // ensure upper bits are zero
        // in_block.data &= decltype(in_block.data)((1<<in_block.num_of_bytes())-1); // ensure upper bits are zero
        #if 1
          for(unsigned i = 0; i < IB; ++i) {
            #pragma HLS UNROLL
            if(i >= in_block.num_of_bytes()) {
              in_block.data((i+1)*8-1,i*8) = 0;
            }
          }
        #else
          // Not working and requires more LUTs
          in_block.data &= decltype(in_block.data)((1<<(in_block.num_of_bytes()*8))-1); 
        #endif

        //Next line: only lower LOG2_OUTPUT_SIZE bits of byte_ptr are selected as  
        // byte_ptr shouldn't be >= OUTPUT_SIZE. This optimizes the resulting HW
        byte_buffer |= decltype(byte_buffer)(in_block.data) << (byte_ptr(LOG2_OUT_WORD_BYTES-1,0).to_int()*8); //OPT
      #else
        if(in_block.num_of_bytes() != 0) {
          // byte_ptr shouldn't be >= OUTPUT_SIZE. This optimizes the HW
          byte_buffer(byte_ptr(LOG2_OUT_WORD_BYTES-1,0) + in_block.num_of_bytes() -1 ,byte_ptr(LOG2_OUTPUT_SIZE-1,0)) 
                    = in_block.data; 
        }
      #endif

      byte_ptr = resulting_byte_ptr;

      if(byte_ptr >= OB) {
        byte_block<OB> out_byte_block;
        out_byte_block.data = byte_buffer;// select lower bits
        out_byte_block.set_num_of_bytes(OB);
        out_byte_block.set_last( byte_ptr==OB? in_block.is_last() : false);

        out_bytes << out_byte_block; 
        byte_ptr -= OB; // OPT: OB is a power of 2, then I can just truncate

        //Next line: equivalent to: byte_buffer >>=OB*8;
        byte_buffer = byte_buffer(BYTE_BUFFER_WIDTH-1,OB*8); 

        // if out_byte_block.last_block ==1. Then, send_remaining_data, byte_ptr, byte_buffer 
        // are all 0, ready for next block (No need to reset them explicitly).
      }

      // send_remaining_data = byte_ptr != 0 ?in_block.last_block : ap_uint<1> (0); 


    }else{ // send the data in the bit_buffer
      ASSERT(byte_ptr,<,OB ); 
      ASSERT(byte_ptr,>,0 ); 
      byte_block<OB> out_byte_block;
      out_byte_block.data = byte_buffer;// select lower bits
      out_byte_block.set_num_of_bytes(byte_ptr);
      out_byte_block.set_last(true);
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

#endif // PACK_OUT_BYTES_HPP