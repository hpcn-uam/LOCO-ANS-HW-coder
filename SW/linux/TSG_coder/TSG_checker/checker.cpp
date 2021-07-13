
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include "bin_decoder.hpp"
#include "checker_config.hpp"
#include <vector>
#include <list>
#include <fstream>

using namespace std;

#define TEST_BUFFER_SIZE 2048
struct global_header {
  uint32_t bytes;
};

struct input_t {
  int z,y,theta_id,p_id,last;
};
int main(int argc, char const *argv[])
{
  
  int num_of_errors = 0;
  if(argc != 4){
    cout<<"Error: need 3 args: in_file block_size and blk_idx"<<endl;
    return 1;
  } 


  const char * in_file = argv[1];
  int block_size = atoi(argv[2]);
  int blk_idx = atoi(argv[3]);
 

  std::vector<input_t> input_vector;
  cout<<"Processing block "<<blk_idx<<"| size: "<<block_size<<endl;


  // ************
  // Generate input 
  // ************
//    int block_size = 50;
  //    
  for (int i = 0; i < block_size; ++i){
    input_t in_elem;
    in_elem.last = (i == block_size-1)? 1:0 ;
    int val = i;
    // val = 0;
    in_elem.z = blk_idx <= 3? val&0x1:(blk_idx <= 5? val & 0xF : val & 0x7F) ;
    in_elem.y = val & 0x80?1:0 ; 
    in_elem.theta_id = i >= NUM_ANS_THETA_MODES?NUM_ANS_THETA_MODES-1: i ;
    // theta_id =10;
    in_elem.p_id = blk_idx/2 ;
    input_vector.push_back(in_elem);

  }




  // ************
  // Check output
  // ************

  int byte_counter = 0; // counter to check last_byte_idx signal
  global_header header;
  std::ifstream binary_in_file(in_file, std::ios::binary );
  binary_in_file.read((char*)&header,sizeof(header));

  cout<<" bytes in file: "<<header.bytes<<endl;

  int buffer_size = header.bytes+8;
  uint8_t* block_binary = new uint8_t[buffer_size]; 
  binary_in_file.read((char*)block_binary,buffer_size*sizeof(*block_binary));
  //read file 



  // Decode and check
  Binary_Decoder bin_decoder(block_binary,block_size);
  int i = 0;
  for (auto elem_it : input_vector){    
    //decode 
    const int remainder_bits = EE_REMAINDER_SIZE - 0;
    int deco_z, deco_y;
    bin_decoder.retrive_TSG_symbol(elem_it.theta_id, 
                elem_it.p_id, remainder_bits, deco_z, deco_y);

    //check
    ASSERT(deco_z,== ,elem_it.z,"Blk: "<<blk_idx<<" | i:"<<i);
    ASSERT(deco_y,== ,elem_it.y,"Blk: "<<blk_idx<<" | i:"<<i);
    i++;

  }

  cout<<"Sucess"<<endl;
  // check all memory words have been used 
  //clean up 
  delete[] block_binary;
  

  return num_of_errors;
}
