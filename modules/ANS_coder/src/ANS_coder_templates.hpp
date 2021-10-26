/*
  Copyright 2021 Tobías Alonso, Autonomous University of Madrid
  This file is part of LOCO-ANS HW encoder.
  LOCO-ANS HW encoder is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  LOCO-ANS HW encoder is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with LOCO-ANS HW encoder.  If not, see <https://www.gnu.org/licenses/>.
*/
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

  pack_out_bits_up<NUM_OUT_OF_BYTES>(bit_block_stream,byte_block_stream);

}

template<unsigned NUM_OUT_OF_BYTES>
void ANS_coder_ext_ROM(
  stream<subsymb_t> &symbol_stream,
  stream<byte_block<NUM_OUT_OF_BYTES>> &byte_block_stream,
  const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2],
  const tANS_table_t  tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]){
  // #pragma HLS INLINE
  
  #pragma HLS DATAFLOW disable_start_propagation
  #pragma HLS INTERFACE ap_ctrl_none port=return
  
  // stream<bit_blocks_with_meta<NUM_ANS_BITS>> out_bit_stream;
  // #pragma HLS STREAM variable=out_bit_stream depth=2
  // code_symbols_ext_ROM(symbol_stream,out_bit_stream,tANS_y_encode_table,tANS_z_encode_table);

  stream<bit_blocks> bit_block_stream;
  #pragma HLS STREAM variable=bit_block_stream depth=2
  code_symbols_ext_ROM(symbol_stream,bit_block_stream,tANS_y_encode_table,tANS_z_encode_table);
  // serialize_last_state(out_bit_stream,bit_block_stream);


  pack_out_bits_up<NUM_OUT_OF_BYTES>(bit_block_stream,byte_block_stream);

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

  #pragma HLS PIPELINE style=flp 
  // #pragma HLS PIPELINE style=flp II=1

  constexpr int OUT_WORD_BITS = NUM_OUT_OF_BYTES*8;
  constexpr int LOG2_OUT_WORD_BITS = ceillog2(OUT_WORD_BITS);
  constexpr int BIT_BUFFER_WIDTH = (8*NUM_OUT_OF_BYTES)+BIT_BLOCK_SIZE;
  ASSERT(OUT_WORD_BITS,>=,BIT_BLOCK_SIZE ); // previous ptr width declaration assumes this
  //state variables
  static ap_uint<1> send_remaining_data = 0; // if 1, send the data in the bit_buffer
  #pragma HLS reset variable=send_remaining_data 
  static ap_uint<LOG2_OUT_WORD_BITS+1> bit_ptr=0;
  #pragma HLS reset variable=bit_ptr 
  static ap_uint<BIT_BUFFER_WIDTH> bit_buffer=0;
  #pragma HLS reset variable=bit_buffer

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
        // this way of masking bits is less efficient although it works
        // better for masking bytes
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

  #pragma HLS PIPELINE style=flp II=1

  constexpr int OUT_WORD_BITS = NUM_OUT_OF_BYTES*8;
  constexpr int LOG2_OUT_WORD_BITS = ceillog2(OUT_WORD_BITS);
  //state variables
  static ap_uint<1> send_remaining_data = 0; // if 1, send the data in the bit_buffer
  #pragma HLS reset variable=send_remaining_data
  static ap_uint<LOG2_OUT_WORD_BITS+1> bit_ptr=0;
  #pragma HLS reset variable=bit_ptr
  ASSERT(OUT_WORD_BITS,>=,BIT_BLOCK_SIZE ); // previous ptr width declaration assumes this
  static ap_uint<OUTPUT_SIZE+BIT_BLOCK_SIZE> bit_buffer=0;
  #pragma HLS reset variable=bit_buffer

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

template<unsigned NUM_OUT_OF_BYTES>
class ANSCoder {
private:
  //code_symbols_ext_ROM   state variables
  ap_uint<NUM_ANS_BITS> ANS_state = INITIAL_ANS_STATE;

  //pack_out_bits_up state variables
  static constexpr int OUT_WORD_BITS = NUM_OUT_OF_BYTES*8;
  static constexpr int LOG2_OUT_WORD_BITS = ceillog2(OUT_WORD_BITS);
  static constexpr int BIT_BUFFER_WIDTH = (8*NUM_OUT_OF_BYTES)+BIT_BLOCK_SIZE;
  ap_uint<1> send_remaining_data = 0; // if 1, send the data in the bit_buffer
  ap_uint<LOG2_OUT_WORD_BITS+1> bit_ptr=0;
  ap_uint<BIT_BUFFER_WIDTH> bit_buffer=0;


  //serialize_last_state state variables
  ap_uint<1> send_output = 0;
  ap_uint<NUM_ANS_BITS> last_ANS_state = 0;

public:
  ANSCoder(){
    ANS_state = INITIAL_ANS_STATE;
    send_remaining_data = 0; // if 1, send the data in the bit_buffer
    bit_ptr=0;
    bit_buffer=0;
    send_output = 0;
    last_ANS_state = 0;
  };

  void code(
    stream<subsymb_t> &symbol_stream,
    stream<byte_block<NUM_OUT_OF_BYTES>> &byte_block_stream,
    const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2],
    const tANS_table_t  tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]){
    #pragma HLS DATAFLOW disable_start_propagation
    #pragma HLS INTERFACE ap_ctrl_none port=return
    // #pragma HLS inline

    stream<bit_blocks_with_meta<NUM_ANS_BITS>> out_bit_stream;
    #pragma HLS STREAM variable=out_bit_stream depth=2
    code_symbols_ext_ROM(symbol_stream,out_bit_stream,tANS_y_encode_table,tANS_z_encode_table);

    stream<bit_blocks> bit_block_stream;
    #pragma HLS STREAM variable=bit_block_stream depth=2
    serialize_last_state(out_bit_stream,bit_block_stream);

    pack_out_bits_up(bit_block_stream,byte_block_stream);
  }

  template<size_t M>
  void tANS_encode(
    tANS_table_t ANS_table_entry,
    ap_uint<NUM_ANS_BITS> &ANS_encoder_state , 
    bit_blocks_with_meta<M> &out_bits){
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

  void code_symbols_ext_ROM(
    stream<subsymb_t> &symbol_stream,
    stream<bit_blocks_with_meta<NUM_ANS_BITS>> &out_bit_stream,
    const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2],
    const tANS_table_t  tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]
    ){

    
    #pragma HLS INTERFACE ap_ctrl_none port=return
     #pragma HLS PIPELINE style=flp II=1
   // #pragma HLS PIPELINE style=flp
    // #pragma HLS latency min=3 
    


    bit_blocks_with_meta<NUM_ANS_BITS> out_bits;

    subsymb_t symbol = symbol_stream.read();  
    // ap_wait();

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
      // out_bits.metadata = 0;
      // out_bits.metadata = ANS_state;
    }else{
      tANS_table_t ANS_table_entry;
      if(symbol.type == SUBSYMB_Y) {
        ANS_table_entry = tANS_y_encode_table[symbol.info][ANS_state][symbol.subsymb[0]];
      }else{
        ANS_table_entry = tANS_z_encode_table[symbol.info][ANS_state][symbol.subsymb(ANS_SUBSYMBOL_BITS-1,0)];
      }
      tANS_encode(ANS_table_entry,ANS_state ,out_bits);
      
      out_bits.metadata = ANS_state;

      if(symbol.end_of_block == 1) {
        ANS_state = INITIAL_ANS_STATE;
      }
    }

    out_bit_stream << out_bits;
    
  }

  void pack_out_bits_up(
    stream<bit_blocks> &bit_block_stream,
    stream<byte_block<NUM_OUT_OF_BYTES>> &out_bitstream){

    #pragma HLS PIPELINE style=flp 
    // #pragma HLS PIPELINE style=flp II=1

    // constexpr int OUT_WORD_BITS = NUM_OUT_OF_BYTES*8;
    // constexpr int LOG2_OUT_WORD_BITS = ceillog2(OUT_WORD_BITS);
    // constexpr int BIT_BUFFER_WIDTH = (8*NUM_OUT_OF_BYTES)+BIT_BLOCK_SIZE;
    ASSERT(OUT_WORD_BITS,>=,BIT_BLOCK_SIZE ); // previous ptr width declaration assumes this

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
          // this way of masking bits is less efficient although it works
          // better for masking bytes
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

  void serialize_last_state(
    stream<bit_blocks_with_meta<NUM_ANS_BITS>> &in_bit_blocks,
    stream<bit_blocks> &out_bit_blocks){

    #if ANS_OUTPUT_TOP
    #pragma HLS INTERFACE axis register_mode=both register port=in_bit_blocks
    #pragma HLS INTERFACE axis register_mode=both register port=out_bit_blocks
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #endif

    #pragma HLS PIPELINE style=flp II=1

    bit_blocks_with_meta<NUM_ANS_BITS> in_block;
    bit_blocks out_block;

    // START OF SW ONLY LOOP. Not needed in HW as it's a free running pipeline
    #ifndef __SYNTHESIS__
    do{
    #endif

      if(send_output == 1) {
        send_output = 0;
        #if SYMBOL_ENDIANNESS_LITTLE
        out_block.data = ( last_ANS_state, ap_uint<1>(1));
        #else
        out_block.data = (ap_uint<1>(1), last_ANS_state);
        #endif
        out_block.bits = NUM_ANS_BITS+1;
        out_block.last_block = 1;
      }else{
        in_block = in_bit_blocks.read();
        send_output = in_block.last_block;
        last_ANS_state = in_block.metadata; // save just in case I need it in next cycle
        in_block.last_block =0 ;
        out_block = in_block;
      }

      out_bit_blocks << out_block;

    // END OF SW ONLY LOOP
    #ifndef __SYNTHESIS__
    }while(send_output == 1) ;
    #endif
  }
};

#endif // ANS_CODER_TEMPLATES_HPP