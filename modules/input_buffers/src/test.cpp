
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>
#include "input_buffers.h"


using namespace std;
using namespace hls;
#define NUM_OF_BLCKS (4)
int main(int argc, char const *argv[])
{
  stream<coder_interf_t> in_data,out_data;
  int num_of_errors = 0;
  for (int blk_idx = 0; blk_idx < NUM_OF_BLCKS; ++blk_idx){
    for (int i = 0; i < BUFFER_SIZE; ++i){
      coder_interf_t symbol;
      if(blk_idx == 0 && i == 0) { // first pixel
        symbol.set_first_px_flag(1);
        pixel_t px = 0xFF;
        symbol.set_data(px);
      }else{
        symbol.set_first_px_flag(0);
        int val = i+BUFFER_SIZE*blk_idx;
        int y = val & 0x80?1:0 ;
        int z = val & 0x7F ;
        int theta = blk_idx ;
        int p = blk_idx/2 ;
        compress_symbol_t compress_symbol(z,y,theta,p);
        symbol.set_data(compress_symbol.to_intf());
      }
      in_data.write(symbol);
    }

    input_buffers(in_data,out_data);
    
    for (int i = BUFFER_SIZE-1; i>=0; --i){
      coder_interf_t symbol = out_data.read();
      if(blk_idx == 0 && i == 0) { // first pixel
        if(symbol.is_first_px() != 1) {
          cout<<"Error: First pixel: i="<<i<<" , symbol.is_first_px()="<<symbol.is_first_px()<<endl;
          num_of_errors++;
        }
        if(symbol.get_data() != 0xFF) {
          cout<<"Error: i="<<i<<" , symbol="<<symbol.get_data()<<endl;
          num_of_errors++;
        }

      }else{
        if(symbol.is_first_px() != 0) {
          cout<<"Error: Not first pixel: i="<<i<<" , symbol.is_first_px()="<<symbol.is_first_px()<<endl;
          num_of_errors++;
        }
        compress_symbol_t compress_symbol(symbol.get_data());
        int val = i+BUFFER_SIZE*blk_idx;
        int y = val & 0x80?1:0 ;
        int z = val & 0x7F ;
        int theta = blk_idx ;
        int p = blk_idx/2 ;

        if(compress_symbol.z != z) {
          cout<<"Error: Z i="<<i<<" | z="<<z <<" ,compress_symbol.z="<<compress_symbol.z<<endl;
          num_of_errors++;
        }

        
      }

    }
  }

  return num_of_errors;
}
