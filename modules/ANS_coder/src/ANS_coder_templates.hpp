#ifndef ANS_CODER_TEMPLATES_HPP
#define ANS_CODER_TEMPLATES_HPP

#include "ANS_coder.hpp" // won't be added cause ANS_CODER_HPP is define but linter needs it
template<unsigned NUM_OUT_OF_BYTES>
void ANS_coder(
  stream<subsymb_t> &symbol_stream,
  stream<byte_block<NUM_OUT_OF_BYTES>> &byte_block_stream){
  // #pragma HLS INLINE
  
  #pragma HLS DATAFLOW disable_start_propagation
  #pragma HLS INTERFACE ap_ctrl_none port=return
  
  stream<bit_blocks_with_meta<NUM_ANS_BITS>> out_bit_stream;
  #pragma HLS STREAM variable=out_bit_stream depth=2
  code_symbols(symbol_stream,out_bit_stream);

  stream<bit_blocks> bit_block_stream;
  #pragma HLS STREAM variable=bit_block_stream depth=2
  serialize_last_state(out_bit_stream,bit_block_stream);

  #if 0
  stream<bit_blocks> bit_block_stream_1,bit_block_stream_2;
  while(!bit_block_stream.empty()) {
    bit_blocks aux = bit_block_stream.read();
    bit_block_stream_1 << aux;
    bit_block_stream_2 << aux;

  }
  stream<byte_block<NUM_OUT_OF_BYTES>> byte_block_stream_1,byte_block_stream_2;
  pack_out_bits_up<NUM_OUT_OF_BYTES>(bit_block_stream_1,byte_block_stream_1);
  pack_out_bits<NUM_OUT_OF_BYTES>(bit_block_stream_2,byte_block_stream_2);
  static int counter = 0,last_counter=0;
  while(!byte_block_stream_1.empty()) {
    byte_block<NUM_OUT_OF_BYTES> byte_block_1 = byte_block_stream_1.read();
    byte_block<NUM_OUT_OF_BYTES> byte_block_2 = byte_block_stream_2.read();

    ap_uint<8* NUM_OUT_OF_BYTES > mask = (ap_uint<8* NUM_OUT_OF_BYTES >(1)<<(byte_block_1.num_of_bytes()*8))-1;
    ap_uint<8* NUM_OUT_OF_BYTES > data1 = byte_block_1.data &mask;
    ap_uint<8* NUM_OUT_OF_BYTES > data2 = byte_block_2.data &mask;

    ASSERT(byte_block_1.num_of_bytes(),==,byte_block_2.num_of_bytes(),"blk:"<<last_counter<< " | counter: "<<counter)
    ASSERT(byte_block_1.is_last(),==,byte_block_2.is_last(),"blk:"<<last_counter<< " | counter: "<<counter)
    ASSERT(data1,==,data2, "blk:"<<last_counter<< " | counter: "<<counter<<"| last: "<<byte_block_1.is_last()<<
        "| bytes: "<<byte_block_1.num_of_bytes())

    byte_block_1.data = data1;
    // byte_block_1.data = byte_block_2.data;
    // byte_block_2.set_num_of_bytes() = byte_block_1.data;
    byte_block_stream << byte_block_1;

    counter++;
    if(byte_block_1.is_last()) {
      last_counter++;
      counter =0;
    }
  }
  #else
  pack_out_bits_up<NUM_OUT_OF_BYTES>(bit_block_stream,byte_block_stream);
  #endif
  // pack_out_bits<NUM_OUT_OF_BYTES>(bit_block_stream,byte_block_stream);
  // pack_out_bits_sw<NUM_OUT_OF_BYTES>(bit_block_stream,byte_block_stream);
  // pack_out_bits(bit_block_stream,byte_block_stream);
}


// This function packs bits into words of the size determined by the configuration
// file.
// Assumtions: 
//   * It assumes that the last bit_block is not empty (.bits == 0)
//     If the last bit_block is empty it fails to finish the stream with a 
//     last terminator. 
//     This function was designed to be used after serialize_last_state which
//     never sends the last bit_block with .bits ==0.
//     If this functionality is needed, add a function before pack_out_bits
//     that removes the trailing empty bit_blocks and sets the last terminator
//     in the last non empty bit_block
//     

template<unsigned NUM_OUT_OF_BYTES>
void pack_out_bits_up(
  stream<bit_blocks> &bit_block_stream,
  stream<byte_block<NUM_OUT_OF_BYTES>> &out_bitstream){

  #pragma HLS PIPELINE style=frp
  constexpr int OUT_WORD_BITS = NUM_OUT_OF_BYTES*8;
  constexpr int LOG2_OUT_WORD_BITS = ceillog2(OUT_WORD_BITS);
  //state variables
  static ap_uint<1> send_remaining_data = 0; // if 1, send the data in the bit_buffer
  static ap_uint<LOG2_OUT_WORD_BITS+1> bit_ptr=0;
  ASSERT(OUT_WORD_BITS,>=,BIT_BLOCK_SIZE ); // previous ptr width declaration assumes this
  constexpr int BIT_BUFFER_WIDTH = OUTPUT_SIZE+BIT_BLOCK_SIZE;
  static ap_uint<BIT_BUFFER_WIDTH> bit_buffer=0;

  // START OF SW ONLY LOOP. Not needed in HW as it's a free running pipeline
  #ifndef __SYNTHESIS__
  while(!bit_block_stream.empty() || send_remaining_data == 1) {
  #endif
    if(send_remaining_data == 0){
      bit_blocks in_block;
      bit_block_stream >> in_block;

      decltype(bit_ptr) resulting_bit_ptr = bit_ptr+in_block.bits;
      ASSERT(bit_ptr,<=,resulting_bit_ptr); // check no overflow
      ASSERT(resulting_bit_ptr,<=,BIT_BUFFER_WIDTH);

      // next conditional is equivalent to: 
      //          resulting_bit_ptr != OUT_WORD_BITS || resulting_bit_ptr != 0 ?
      // Which is equivalent to: 
      //          bit_ptr>0?, after the if(bit_ptr >= OUT_WORD_BITS) {} block
      send_remaining_data = resulting_bit_ptr(LOG2_OUT_WORD_BITS-1,0) != 0 ? 
                                              in_block.last_block : ap_uint<1> (0);

      #if SYMBOL_ENDIANNESS_LITTLE

        //this
        #if 0
        for(unsigned i = 0; i < decltype(in_block.data)::width; ++i) {
          #pragma HLS UNROLL
          if(i >= in_block.bits) {
            in_block.data[i] = 0;
          }
        }
        #else                                   
        in_block.data &= decltype(in_block.data)((1<<in_block.bits)-1); // ensure upper bits are zero
        #endif
        bit_buffer |= decltype(bit_buffer)(in_block.data) << (BIT_BUFFER_WIDTH-in_block.bits -bit_ptr(LOG2_OUT_WORD_BITS-1,0)) ; 
        //or 
        /*if(in_block.bits != 0) {
        bit_buffer(in_block.bits-1, 0) = in_block.data; 
        }*/
      #else
        // assign values to the bit buffer is expensive
        // in_block.data(in_block.data.length()-1,in_block.bits) = 0; // ensure upper bits are zero
        in_block.data &= decltype(in_block.data)((1<<in_block.bits)-1); // ensure upper bits are zero

        //Next line: only lower LOG2_OUT_WORD_BITS bits of bit_ptr are selected as  
        // bit_ptr shouldn't be >= OUT_WORD_BITS. This optimizes the resulting HW
        bit_buffer |= decltype(bit_buffer)(in_block.data) << bit_ptr(LOG2_OUT_WORD_BITS-1,0); 
      #endif
      // cout<<" UP: buffer: " <<bit_buffer.to_string(2)<<"| new ptr:"<<resulting_bit_ptr<<endl; 

      bit_ptr = resulting_bit_ptr;

      if(bit_ptr >= OUT_WORD_BITS) {
        byte_block<NUM_OUT_OF_BYTES> out_byte_block;

        bit_ptr -= OUT_WORD_BITS; // OPT: OUT_WORD_BITS is a power of 2, then I can just truncate
        #if SYMBOL_ENDIANNESS_LITTLE
        out_byte_block.data = bit_buffer(BIT_BUFFER_WIDTH-1,BIT_BUFFER_WIDTH-OUT_WORD_BITS) ;
        bit_buffer <<=OUT_WORD_BITS;
        // bit_buffer = (bit_buffer(BIT_BUFFER_WIDTH-OUT_WORD_BITS-1,0),ap_uint<OUT_WORD_BITS>(0) );
        #else
        out_byte_block.data = bit_buffer ;// select lower bits
        //Next line: equivalent to: bit_buffer >>=OUT_WORD_BITS;
        bit_buffer = bit_buffer(BIT_BUFFER_WIDTH-1,OUT_WORD_BITS); 
        #endif
        out_byte_block.set_num_of_bytes(NUM_OUT_OF_BYTES);
        out_byte_block.set_last( bit_ptr == 0? in_block.is_last() : false);

        out_bitstream << out_byte_block; 


        // if out_byte_block.last_block ==1. Then, send_remaining_data, bit_ptr, bit_buffer 
        // are all 0, ready for next block (No need to reset them explicitly).
      }

      // send_remaining_data = bit_ptr != 0 ?in_block.last_block : ap_uint<1> (0); 


    }else{ // send the data in the bit_buffer
      ASSERT(bit_ptr,<,OUT_WORD_BITS ); 
      ASSERT(bit_ptr,>,0 ); 
      byte_block<NUM_OUT_OF_BYTES> out_byte_block;
      ap_uint<LOG2_OUT_WORD_BITS+1> aux_ptr = bit_ptr+7;
      if(NUM_OUT_OF_BYTES == 1) { // constexpr compiler should simplify
        out_byte_block.set_num_of_bytes(1);
      }else{
        out_byte_block.set_num_of_bytes(aux_ptr(LOG2_OUT_WORD_BITS,3) );
      }
      #if SYMBOL_ENDIANNESS_LITTLE
      if(NUM_OUT_OF_BYTES == 1) { // constexpr compiler should simplify
        out_byte_block.data = bit_buffer(BIT_BUFFER_WIDTH-1,BIT_BUFFER_WIDTH-OUT_WORD_BITS);
      }else{
        out_byte_block.data = bit_buffer(BIT_BUFFER_WIDTH-1,BIT_BUFFER_WIDTH-OUT_WORD_BITS)>>(aux_ptr(LOG2_OUT_WORD_BITS-1,3)*8) ;
      }
      #else
      out_byte_block.data = bit_buffer;// select lower bits
      #endif
      out_byte_block.set_last(true);
      out_bitstream << out_byte_block; 

      // reset
      send_remaining_data = 0; 
      bit_ptr = 0;
      bit_buffer=0;
    }

  // END OF SW ONLY LOOP. 
  #ifndef __SYNTHESIS__
  }
  #endif

}


template<unsigned NUM_OUT_OF_BYTES>
void pack_out_bits(
  stream<bit_blocks> &bit_block_stream,
  stream<byte_block<NUM_OUT_OF_BYTES>> &out_bitstream){

  #pragma HLS PIPELINE style=frp
  constexpr int OUT_WORD_BITS = NUM_OUT_OF_BYTES*8;
  constexpr int LOG2_OUT_WORD_BITS = ceillog2(OUT_WORD_BITS);
  //state variables
  static ap_uint<1> send_remaining_data = 0; // if 1, send the data in the bit_buffer
  static ap_uint<LOG2_OUT_WORD_BITS+1> bit_ptr=0;
  ASSERT(OUT_WORD_BITS,>=,BIT_BLOCK_SIZE ); // previous ptr width declaration assumes this
  static ap_uint<OUTPUT_SIZE+BIT_BLOCK_SIZE> bit_buffer=0;

  // START OF SW ONLY LOOP. Not needed in HW as it's a free running pipeline
  #ifndef __SYNTHESIS__
  while(!bit_block_stream.empty() || send_remaining_data == 1) {
  #endif
    if(send_remaining_data == 0){
      bit_blocks in_block;
      bit_block_stream >> in_block;

      decltype(bit_ptr) resulting_bit_ptr = bit_ptr+in_block.bits;
      ASSERT(bit_ptr,<=,resulting_bit_ptr); // check no overflow
      ASSERT(resulting_bit_ptr,<=,decltype(bit_buffer)::width);

      // next conditional is equivalent to: 
      //          resulting_bit_ptr != OUT_WORD_BITS || resulting_bit_ptr != 0 ?
      // Which is equivalent to: 
      //          bit_ptr>0?, after the if(bit_ptr >= OUT_WORD_BITS) {} block
      send_remaining_data = resulting_bit_ptr(LOG2_OUT_WORD_BITS-1,0) != 0 ? 
                                              in_block.last_block : ap_uint<1> (0);

      #if SYMBOL_ENDIANNESS_LITTLE
        bit_buffer <<= in_block.bits;

        //this
        in_block.data &= decltype(in_block.data)((1<<in_block.bits)-1); // ensure upper bits are zero
        bit_buffer |= decltype(bit_buffer)(in_block.data) ; 
        //or 
        /*if(in_block.bits != 0) {
        bit_buffer(in_block.bits-1, 0) = in_block.data; 
        }*/
      #else
        // assign values to the bit buffer is expensive
        // in_block.data(in_block.data.length()-1,in_block.bits) = 0; // ensure upper bits are zero
        in_block.data &= decltype(in_block.data)((1<<in_block.bits)-1); // ensure upper bits are zero

        //Next line: only lower LOG2_OUT_WORD_BITS bits of bit_ptr are selected as  
        // bit_ptr shouldn't be >= OUT_WORD_BITS. This optimizes the resulting HW
        bit_buffer |= decltype(bit_buffer)(in_block.data) << bit_ptr(LOG2_OUT_WORD_BITS-1,0); 
      #endif

      // cout<<" DW: buffer: " <<bit_buffer.to_string(2)<<"| new ptr:"<<resulting_bit_ptr<<endl; 

      bit_ptr = resulting_bit_ptr;

      if(bit_ptr >= OUT_WORD_BITS) {
        byte_block<NUM_OUT_OF_BYTES> out_byte_block;

        bit_ptr -= OUT_WORD_BITS; // OPT: OUT_WORD_BITS is a power of 2, then I can just truncate
        #if SYMBOL_ENDIANNESS_LITTLE
        out_byte_block.data = bit_buffer>>bit_ptr(LOG2_OUT_WORD_BITS-1,0); ;
        #else
        out_byte_block.data = bit_buffer ;// select lower bits
        //Next line: equivalent to: bit_buffer >>=OUT_WORD_BITS;
        bit_buffer = bit_buffer(decltype(bit_buffer)::width-1,OUT_WORD_BITS); 
        #endif
        out_byte_block.set_num_of_bytes(NUM_OUT_OF_BYTES);
        out_byte_block.set_last( bit_ptr == 0? in_block.is_last() : false);

        out_bitstream << out_byte_block; 


        // if out_byte_block.last_block ==1. Then, send_remaining_data, bit_ptr, bit_buffer 
        // are all 0, ready for next block (No need to reset them explicitly).
      }

      // send_remaining_data = bit_ptr != 0 ?in_block.last_block : ap_uint<1> (0); 


    }else{ // send the data in the bit_buffer
      ASSERT(bit_ptr,<,OUT_WORD_BITS ); 
      ASSERT(bit_ptr,>,0 ); 
      byte_block<NUM_OUT_OF_BYTES> out_byte_block;
      #if SYMBOL_ENDIANNESS_LITTLE
      ap_uint<3> padding = 8 - bit_ptr(2,0);
      out_byte_block.data = bit_buffer<<padding ;// select lower bits
      #else
      out_byte_block.data = bit_buffer;// select lower bits
      #endif
      ap_uint<LOG2_OUT_WORD_BITS+1> aux_ptr = bit_ptr+7;
      if(NUM_OUT_OF_BYTES == 1) { // constexpr compiler should simplify
        out_byte_block.set_num_of_bytes(1);
      }else{
        out_byte_block.set_num_of_bytes(aux_ptr(LOG2_OUT_WORD_BITS,3) );
      }
      out_byte_block.set_last(true);
      out_bitstream << out_byte_block; 

      // reset
      send_remaining_data = 0; 
      bit_ptr = 0;
      bit_buffer=0;
    }

  // END OF SW ONLY LOOP. 
  #ifndef __SYNTHESIS__
  }
  #endif

}


template<unsigned NUM_OUT_OF_BYTES>
void pack_out_bits_sw(
  stream<bit_blocks> &bit_block_stream,
  stream<byte_block<NUM_OUT_OF_BYTES>> &out_bitstream){
  constexpr int OUT_WORD_BITS = NUM_OUT_OF_BYTES*8;
  constexpr int LOG2_OUT_WORD_BITS = ceillog2(OUT_WORD_BITS);

  //state variables
  // static uint bit_ptr=0;
  static ap_uint<LOG2_OUT_WORD_BITS+1> bit_ptr=0;
  ASSERT(OUT_WORD_BITS,>=,BIT_BLOCK_SIZE ); // previous ptr width declaration assumes this
  // static long long unsigned bit_buffer=0;
  static ap_uint<OUT_WORD_BITS+BIT_BLOCK_SIZE> bit_buffer=0;


  while(!bit_block_stream.empty()) {
    bit_blocks in_block;
    bit_block_stream >> in_block;

    // in_block.data(in_block.data.length()-1,in_block.bits) = 0; // ensure upper bits are zero
    #if SYMBOL_ENDIANNESS_LITTLE
    bit_buffer <<= in_block.bits;

    //this
    in_block.data &= decltype(in_block.data)((1<<in_block.bits)-1); // ensure upper bits are zero
    bit_buffer |= decltype(bit_buffer)(in_block.data) ; 
    //or 
    // bit_buffer(in_block.bits-1, 0) = in_block.data; 
    #else
    in_block.data &= decltype(in_block.data)((1<<in_block.bits)-1); // ensure upper bits are zero
    bit_buffer |= decltype(bit_buffer)(in_block.data) << bit_ptr; 
    #endif

    ASSERT(bit_ptr,<=,bit_ptr+in_block.bits.to_int()); // check no overflow
    bit_ptr += in_block.bits;
    ASSERT(bit_ptr,<=,OUT_WORD_BITS+BIT_BLOCK_SIZE);

    byte_block<NUM_OUT_OF_BYTES> out_byte_block;
    if(bit_ptr >= OUT_WORD_BITS) {
      bit_ptr -= OUT_WORD_BITS; // OPT: OUT_WORD_BITS is a power of 2, then I can just truncate
      #if SYMBOL_ENDIANNESS_LITTLE
      out_byte_block.data = bit_buffer>>bit_ptr ;// select lower bits
      #else
      out_byte_block.data = bit_buffer ;// select lower bits
      bit_buffer >>=OUT_WORD_BITS;
      #endif
      out_byte_block.set_num_of_bytes(NUM_OUT_OF_BYTES);
      out_byte_block.set_last(bit_ptr == 0? in_block.is_last() : false );

      out_bitstream << out_byte_block; 
    }

    ASSERT(bit_ptr,<,OUT_WORD_BITS ); 
    if(in_block.last_block == 1 && bit_ptr >0){ // send the data in the bit_buffer
      byte_block<NUM_OUT_OF_BYTES> out_byte_block;
      #if SYMBOL_ENDIANNESS_LITTLE
      ap_uint<3> padding = 8 - bit_ptr(2,0);
      out_byte_block.data = bit_buffer<<padding ;// select lower bits
      #else
      out_byte_block.data = bit_buffer;// select lower bits
      #endif
      uint aux_ptr = bit_ptr+7;
      out_byte_block.set_num_of_bytes( aux_ptr>>3);
      out_byte_block.set_last(true);
      out_bitstream << out_byte_block; 

      // reset
      bit_ptr = 0;
      bit_buffer=0;
    }
  }

}


#endif // ANS_CODER_TEMPLATES_HPP