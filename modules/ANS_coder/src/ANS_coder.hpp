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

#ifndef ANS_CODER_HPP
#define ANS_CODER_HPP

#include <hls_stream.h>
#include <tgmath.h>
#include "../../coder_config.hpp"
#include "../../subsym_gen/src/subsym_gen.hpp"
#include "../../input_buffers/src/input_buffers.hpp"

// #define DEBUG

struct tANS_table_t {
  ap_uint<NUM_ANS_BITS> state;
  ap_uint<LOG2_ANS_BITS> bits;
} ;

struct bit_blocks {
  // gcc allows the following commented declaration, VITIS HLS doesn't 
  // static constexpr size_t _LOG2_BIT_BLOCK_SIZE = size_t(ceil(log2(BIT_BLOCK_SIZE)));
  ap_uint<BIT_BLOCK_SIZE> data;
  ap_uint<LOG2_BIT_BLOCK_SIZE> bits;
  // ap_uint<_LOG2_BIT_BLOCK_SIZE> bits;
  ap_uint<1> last_block;

  inline bool is_last(){
    return last_block == 1;
  }

} ;

template <size_t WMeta>
struct bit_blocks_with_meta : public bit_blocks{
  ap_uint<WMeta> metadata;
} ;


typedef ap_uint<OUTPUT_SIZE> out_word_t;
template<unsigned NUM_OF_BYTES>
struct byte_block {
  //+1 to allow 0 valid bytes
  ap_uint<NUM_OF_BYTES*8> data;
  
  static constexpr int COUNTER_WIDTH = ceillog2(NUM_OF_BYTES+1);
  private:
  ap_uint<COUNTER_WIDTH> bytes;
  ap_uint<1> last;

  public:
  byte_block():data(0),bytes(0),last(0){};
  byte_block( 
    ap_uint<NUM_OF_BYTES*8> _data, ap_uint<COUNTER_WIDTH> _bytes,
    ap_uint<1>  _last): data(_data),bytes(_bytes),last(_last){};

  inline unsigned get_byte_mask(){
    return (1<<bytes)-1;
  }

  inline unsigned num_of_bytes(){
    return bytes;
  }

  inline void set_num_of_bytes(unsigned _bytes){
    bytes = _bytes;
  }

  inline void increment_num_of_bytes(){
    bytes++;
  }

  inline bool is_last(){
    return last == 1;
  }
  
  inline void set_last(bool _last){
    last = _last? 1: 0;
  }

  byte_block & operator=(const byte_block & _block){
    data = _block.data;
    bytes = _block.bytes;
    last = _block.last;
    return *this;
  }

  ap_uint<8> operator()(const unsigned byte_idx){
    return data((byte_idx+1)*8-1,byte_idx*8);
  }

  friend bool operator==(const byte_block<NUM_OF_BYTES> &l,
    const byte_block<NUM_OF_BYTES> &r){
    return  (l.data == r.data) && (l.bytes == r.bytes) && (l.last == r.last);

  }
} ;


template<unsigned NUM_OUT_OF_BYTES>
void ANS_coder(
  stream<subsymb_t> &symbol_stream,
  stream<byte_block<NUM_OUT_OF_BYTES>> &byte_block_stream);

void code_symbols(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks_with_meta<NUM_ANS_BITS>> &out_bit_stream);


void code_symbols_ext_ROM(
  stream<subsymb_t> &symbol_stream,
  stream<bit_blocks> &out_bit_stream,
  const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2],
  const tANS_table_t  tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]);

void serialize_last_state(
    stream<bit_blocks_with_meta<NUM_ANS_BITS>> &in_bit_blocks,
    stream<bit_blocks> &out_bit_blocks);

template<unsigned NUM_OUT_OF_BYTES>
void pack_out_bits(
  stream<bit_blocks> &bit_block_stream,
  stream<byte_block<NUM_OUT_OF_BYTES>> &out_bitstream);

template<unsigned NUM_OUT_OF_BYTES>
void pack_out_bits_up(
  stream<bit_blocks> &bit_block_stream,
  stream<byte_block<NUM_OUT_OF_BYTES>> &out_bitstream);

template<unsigned NUM_OUT_OF_BYTES>
void pack_out_bits_sw(
  stream<bit_blocks> &bit_block_stream,
  stream<byte_block<NUM_OUT_OF_BYTES>> &out_bitstream);


#include "ANS_coder_templates.hpp"

void ANS_coder_top(
  stream<subsymb_t> &symbol_stream,
  stream<byte_block<OUT_WORD_BYTES>> &byte_block_stream);


void ANS_coder_ext_ROM_top(
  stream<subsymb_t> &symbol_stream,
  stream<byte_block<OUT_WORD_BYTES>> &byte_block_stream,
  const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2],
  const tANS_table_t  tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]);


void ANS_coder_double_lane(
  stream<subsymb_t> &symbol_stream_0,
  stream<byte_block<OUT_WORD_BYTES>> &byte_block_stream_0,
  stream<subsymb_t> &symbol_stream_1,
  stream<byte_block<OUT_WORD_BYTES>> &byte_block_stream_1);

#endif // ANS_CODER_HPP