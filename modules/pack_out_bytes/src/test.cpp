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

#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include "./pack_out_bytes.hpp"
#include "../../test/test.hpp"
#include "../../coder_config.hpp"
#include <list>

using namespace std;
using namespace hls;
#define NUM_OF_TESTS (6)
constexpr int MAX_TEST_VECTOR_SIZE = 64;

/*void pack_out_bytes_hw_simplified(
  stream<byte_block> &in_bytes,
  stream<byte_block> &out_bitstream);*/

/*void pack_out_bytes_sw_big_endian(
  stream<byte_block> &in_bytes,
  stream<byte_block> &out_bitstream);
*/
// void pack_out_bytes_sw_little_endian(
//   stream<byte_block> &in_bytes,
//   stream<byte_block> &out_bitstream);

template<unsigned IB>
void test_vector_gen(std::vector<byte_block<IB> > &test_vec,int test);

template<unsigned B>
long long unsigned update_checksum(long long unsigned chksum,byte_block<B> new_elem){
  for(unsigned i = 1; i <= new_elem.num_of_bytes() ; ++i) {
    chksum += new_elem.data(i*8-1,(i-1)*8) &0xFF;
    while(chksum >= 1<<8) {
      chksum = (chksum & 0XFF)+(chksum >>8);
    } 
  }
  return chksum;
}


int main(int argc, char const *argv[])
{
  
  for (int test_idx = 0; test_idx < NUM_OF_TESTS; ++test_idx){
    stream<byte_block<OUT_WORD_BYTES>> in_hw_data,in_sw_data;
    cout<<"Processing test "<<test_idx;
    int block_size = OUTPUT_STACK_SIZE - int(test_idx/2);
    
    //generate test vector
    std::vector<byte_block<OUT_WORD_BYTES>> test_vec;
    test_vector_gen(test_vec, test_idx);
    std::list<ap_uint<8>> golden_bytes;

    // populate hls streams and compute checksum
    long long unsigned golden_checksum = 0;
    for(auto&& in_elem : test_vec) {
      golden_checksum = update_checksum(golden_checksum, in_elem);
      for(unsigned i = 0; i <in_elem.num_of_bytes() ; ++i) {
        golden_bytes.push_back(in_elem.data((i+1)*8-1,i*8) );
      }
      in_hw_data << in_elem;
      in_sw_data << in_elem;
    }

    // Run DUT and sw version
    // stream<byte_block> out_sw_data;
    // stream<byte_block>  &out_hw_data = out_sw_data;
    stream<byte_block<OUT_DMA_BYTES>> out_hw_data,out_sw_data;
    pack_out_bytes_top(in_hw_data,out_hw_data);
    pack_out_bytes_sw_little_endian(in_sw_data,out_sw_data);
    

    // Check output
    ASSERT(out_hw_data.size(),==,out_sw_data.size());

    long long unsigned sw_checksum = 0,hw_checksum = 0;
    int i = 0;
    auto golden_byte_iter = golden_bytes.begin();
    while(!out_hw_data.empty()) {
      auto out_sw_elem = out_sw_data.read();
      // byte_block<OUT_DMA_BYTES> out_sw_elem = out_sw_data.read();
      auto out_hw_elem = out_hw_data.read();
      // byte_block<OUT_DMA_BYTES> out_hw_elem = out_hw_data.read();
      // byte_block out_hw_elem = out_sw_elem;

      ASSERT(out_hw_elem.num_of_bytes(),==,out_sw_elem.num_of_bytes()," | i:"<<i)
      ASSERT(out_hw_elem.is_last(),==,out_sw_elem.is_last()," | i:"<<i)
      ASSERT(out_hw_elem.data,==,out_sw_elem.data," | i:"<<i)

      sw_checksum = update_checksum(sw_checksum ,out_sw_elem );
      hw_checksum = update_checksum(hw_checksum ,out_hw_elem );

      if(!out_hw_data.empty()) {
        ASSERT(out_hw_elem.num_of_bytes(),==,OUT_DMA_BYTES," | i:"<<i)
        ASSERT(out_hw_elem.is_last(),==,false," | i:"<<i)
      }else{
        ASSERT(out_hw_elem.is_last(),==,true," | i:"<<i)
      }

      for(unsigned j = 0; j <out_hw_elem.num_of_bytes() ; ++j) {
        ap_uint<8> out_byte = out_hw_elem.data((j+1)*8-1,j*8);
        ASSERT(out_byte,==,*golden_byte_iter," | i:"<<i<<" | byte:"<<j);
        golden_byte_iter++;
      }

      i++;
    }
    ASSERT(golden_byte_iter == golden_bytes.end() );//check all bytes where packed
    ASSERT(sw_checksum,==,golden_checksum )
    ASSERT(hw_checksum,==,golden_checksum )
    cout<<"| SUCCESS"<<endl;
  }

  return 0;
}

#if 0
int test_sw_version()
{
  
  for (int test_idx = 0; test_idx < NUM_OF_TESTS; ++test_idx){
    stream<byte_block> in_hw_data,in_sw_data;
    cout<<"Processing test "<<test_idx;
    int block_size = OUTPUT_STACK_SIZE - int(test_idx/2);
    
    //generate test vector
    std::vector<byte_block> test_vec;
    test_vector_gen(test_vec, test_idx);
    std::list<ap_uint<8>> golden_bytes;

    // populate hls streams and compute checksum
    long long unsigned golden_checksum = 0;
    for(auto&& in_elem : test_vec) {
      golden_checksum = update_checksum(golden_checksum, in_elem);
      for(unsigned i = 0; i <in_elem.bytes ; ++i) {
        golden_bytes.push_back(in_elem.data((i+1)*8-1,i*8) );
      }
      // in_hw_data << in_elem;
      in_sw_data << in_elem;
    }

    // Run DUT and sw version
    stream<byte_block> out_sw_data;
    stream<byte_block>  &out_hw_data = out_sw_data;
    // stream<byte_block> out_hw_data,out_sw_data;
    // pack_out_bytes(in_hw_data,out_hw_data);
    pack_out_bytes_sw_little_endian(in_sw_data,out_sw_data);
    

    // Check output
    ASSERT(out_hw_data.size(),==,out_sw_data.size());

    long long unsigned sw_checksum = 0,hw_checksum = 0;
    int i = 0;
    auto golden_byte_iter = golden_bytes.begin();
    while(!out_hw_data.empty()) {
      byte_block out_sw_elem = out_sw_data.read();
      // byte_block out_hw_elem = out_hw_data.read();
      byte_block out_hw_elem = out_sw_elem;

      ASSERT(out_hw_elem.data,==,out_sw_elem.data," | i:"<<i)
      ASSERT(out_hw_elem.bytes,==,out_sw_elem.bytes," | i:"<<i)
      ASSERT(out_hw_elem.last_block,==,out_sw_elem.last_block," | i:"<<i)

      sw_checksum = update_checksum(sw_checksum ,out_sw_elem );
      hw_checksum = update_checksum(hw_checksum ,out_hw_elem );

      if(!out_hw_data.empty()) {
        ASSERT(out_hw_elem.bytes,==,OUT_WORD_BYTES," | i:"<<i)
        ASSERT(out_hw_elem.last_block,==,0," | i:"<<i)
      }else{
        ASSERT(out_hw_elem.last_block,==,1," | i:"<<i)
      }

      for(unsigned j = 0; j <out_hw_elem.bytes ; ++j) {
        ap_uint<8> out_byte = out_hw_elem.data((j+1)*8-1,j*8);
        ASSERT(out_byte,==,*golden_byte_iter," | i:"<<i<<" | byte:"<<j);
        golden_byte_iter++;
      }


      /*big endian
      for(unsigned j = out_hw_elem.bytes; j > 0 ; --j) {
        ap_uint<8> out_byte = out_hw_elem.data(j*8-1,(j-1)*8);
        ASSERT(out_byte,==,*golden_byte_iter," | i:"<<i);
        golden_byte_iter++;
      }*/

      i++;
    }
    ASSERT(golden_byte_iter == golden_bytes.end() );//check all bytes where packed
    ASSERT(sw_checksum,==,golden_checksum )
    ASSERT(hw_checksum,==,golden_checksum )
    cout<<"| SUCCESS"<<endl;
  }

  return 0;
}
#endif

template<unsigned IB>
void test_vector_gen(std::vector<byte_block<IB> > &test_vec,int test){
  switch(test){
    case 0:
      {
      cout<<" | Test full words with small numbers ";
    
      //generate data
      int block_size = MAX_TEST_VECTOR_SIZE;
      for (int i = 0; i < block_size; ++i){
        byte_block<IB> in_elem;
        in_elem.data = i ;//| (i<<16);
        in_elem.set_num_of_bytes( OUT_WORD_BYTES );
        in_elem.set_last(i == block_size-1);
        test_vec.push_back(in_elem);
      }
      }
      break;
    case 1:
      {
      cout<<" | Test full words with big numbers ";
      //generate data
      int block_size = MAX_TEST_VECTOR_SIZE;
      for (int i = 0; i < block_size; ++i){
        byte_block<IB> in_elem;
        in_elem.data = i | (i<<16);
        in_elem.set_num_of_bytes( OUT_WORD_BYTES );
        in_elem.set_last(i == block_size-1);
        test_vec.push_back(in_elem);
      }
      }
      break;

    case 2:
    case 3:
    case 4:
    case 5:
      {
      cout<<" | Test variable size (non empty) words with big numbers ";
      int block_size = MAX_TEST_VECTOR_SIZE - test;
    
      //generate data
      for (int i = 0; i < block_size; ++i){
        byte_block<IB> in_elem;
        in_elem.data = i | (i<<16);
        in_elem.set_num_of_bytes( (i%(OUT_WORD_BYTES))+1 );
        in_elem.set_last(i == block_size-1);
        test_vec.push_back(in_elem);
      }
      }
      break;

    default:
      cerr<<"Not a valid test number"<<endl;
      throw 1;
      break;
  }
}



/*void pack_out_bytes_hw_simplified(
  stream<byte_block> &in_bytes,
  stream<byte_block> &out_bitstream){

  //state variables
  static uint byte_ptr=0;
  static long long unsigned byte_buffer=0;

  while(!in_bytes.empty()) {
    byte_block in_block;
    in_bytes >> in_block;

    in_block.data &= decltype(in_block.data)((1<<(in_block.bytes*8))-1); //TODO:  BUGGY// ensure upper bits are zero
    byte_buffer |= decltype(byte_buffer)(in_block.data) << (byte_ptr*8); 

    byte_ptr += in_block.bytes;
    ASSERT(byte_ptr,<=,sizeof(byte_buffer)); // check no overflow

    byte_block out_byte_block;
    if(byte_ptr >= OUT_WORD_BYTES) {
      out_byte_block.data = out_word_t(byte_buffer);// select lower bits
      out_byte_block.bytes = OUT_WORD_BYTES;
      out_byte_block.last_block = byte_ptr == OUT_WORD_BYTES? in_block.last_block : ap_uint<1> (0);

      out_bitstream << out_byte_block; 
      byte_ptr -= OUT_WORD_BYTES;
      byte_buffer >>=OUT_WORD_BYTES*8;
    }

    ASSERT(byte_ptr,<,OUT_WORD_BYTES ); 
    if(in_block.last_block == 1 && byte_ptr >0){ // send the data in the byte_buffer
      byte_block out_byte_block;
      out_byte_block.data = out_word_t(byte_buffer);// select lower bits
      out_byte_block.bytes = byte_ptr;
      out_byte_block.last_block = 1;
      out_bitstream << out_byte_block; 

      // reset
      byte_ptr = 0;
      byte_buffer=0;
    }

    if(in_block.last_block == 1) {
      ASSERT(byte_ptr,==,0)
      ASSERT(byte_buffer,==,0)
    }
  }

}*/
