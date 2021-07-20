
#include <iostream>
#include <stdlib.h>     /* srand, rand */
#include <cmath>
#include <ctime>
#include <cstring>
#include <vector>
#include <list>
#include <functional>
#include "../../test/test.hpp"
#include "LOCO_decorrelator.hpp"
#include "sw_implementation.hpp"

using namespace std;
using namespace hls;

// #define PRINT_INPUT

#define ROWS 128
#define COLS 128
const int NUM_OF_TESTS = 6;

int main(int argc, char const *argv[])
{
  int near = 0;
  hls::stream<px_t> in_px_stream;
  hls::stream<err_t> out_symbol_stream;

  hls::stream<int> sw_in_stream, sw_out_symbol_stream;

  for(unsigned test_idx = 0; test_idx < NUM_OF_TESTS; ++test_idx) {
    int img_rows = ROWS+test_idx;
    img_rows= img_rows > MAX_ROWS? MAX_ROWS:img_rows;
    int img_cols = COLS - test_idx;
    img_cols= img_cols < 5? 5:img_cols;
    
    constexpr int INPUT_MASK = (1<< INPUT_BPP)-1;
    std::function<px_t(int,int)> px_gen;
    std::string test_name;
    if(test_idx == 0) {
      px_gen = [&](int row,int col) {return 120;};
      test_name = "Constant image";
    }else if(test_idx == 1) {
      px_gen = [&](int row,int col) {return col & INPUT_MASK;};
      // px_gen = [](int row,int col) {return col & INPUT_MASK;};
      test_name = "Grad 0";
    }else if(test_idx == 2) {
      px_gen = [&](int row,int col) {return (col+row) & INPUT_MASK;};
      test_name = "Grad 1";
    }else if(test_idx == 3) {
      px_gen = [&](int row,int col) {return (col+row*img_cols) & INPUT_MASK;};
      test_name = "Grad 2";
    }else{
      srand (test_idx); // use test_idx as seed
      px_gen = [&](int row,int col){return rand() & INPUT_MASK  ;};
      test_name = "Random image";
    }

    cout<<"Test "<<test_idx<<"| "+test_name+" |"<<" Image size:" <<img_rows
        <<"x"<<img_cols<<endl;
    
    
    for (int row = 0; row < img_rows; ++row){
      for (int col = 0; col < img_cols; ++col){
        px_t in_px = px_gen(row,col);

        in_px_stream<<in_px;
        sw_in_stream<<in_px;

        #ifdef PRINT_INPUT
        printf("%3d|",int(in_px));
        #endif
      }
      #ifdef PRINT_INPUT
      cout<<endl;
      #endif
    }

    ASSERT(in_px_stream.size(),==,img_rows*img_cols);
    ASSERT(sw_in_stream.size(),==,img_rows*img_cols);

    LOCO_decorrelator(img_rows,img_cols, in_px_stream, out_symbol_stream);

    sw_impl::image_scanner(near, img_rows, img_cols, sw_in_stream, sw_out_symbol_stream);

    ASSERT(out_symbol_stream.size(),==,sw_out_symbol_stream.size());


    for (int row = 0; row < img_rows; ++row){
      for (int col = 0; col < img_cols; ++col){
        auto hw_out = out_symbol_stream.read();
        auto sw_out = sw_out_symbol_stream.read();

        ASSERT(hw_out,==,sw_out,"row,col:("<<row<<","<<col<<")")
      }
    }

    ASSERT(in_px_stream.size(),==,0)
    ASSERT(out_symbol_stream.size(),==,0)

    cout<<"\t|Success "<<endl;
  } // for test_idx
  return 0;
}