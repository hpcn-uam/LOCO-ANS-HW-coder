
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
using namespace sw_impl;

// #define PRINT_INPUT

#define ROWS 10
#define COLS 10
// #define ROWS 128
// #define COLS 128
const int NUM_OF_TESTS = 6;

int main(int argc, char const *argv[])
{
  int near = 0;
  hls::stream<px_t> in_px_stream,out_first_px;
  
  hls::stream<coder_interf_t>  out_symbol_stream;

  hls::stream<int> sw_in_stream;
  stream<ee_symb_data> sw_out_symbol_stream;

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
    // int number_of_images = 1;
    #if 1
    hls::stream<DecorrelatorOutput> out_presymbol_stream;
    LOCO_decorrelator(img_rows,img_cols, in_px_stream, out_first_px,out_presymbol_stream);
    
    while(!out_presymbol_stream.empty()) {
    // for(unsigned i = 0; i < img_rows*img_rows-1; ++i) {
      St_idx_compute(out_presymbol_stream,out_symbol_stream);
    }
    #else
    LOCO_decorrelator(img_rows,img_cols, in_px_stream, out_first_px,out_symbol_stream);
    #endif

    sw_impl::image_scanner(near, img_rows, img_cols, sw_in_stream, sw_out_symbol_stream);

    // ASSERT(out_presymbol_stream.size(),==,0);
    ASSERT(out_symbol_stream.size()+1,==,sw_out_symbol_stream.size());
    ASSERT(out_first_px.size(),==,1);


    for (int row = 0; row < img_rows; ++row){
      for (int col = 0; col < img_cols; ++col){

        auto sw_out = sw_out_symbol_stream.read();
        int golden_last = (row == (img_rows-1)) && (col == (img_cols-1))?1:0; 
        if(row == 0 && col == 0) {
          auto hw_1px_out = out_first_px.read();
          ASSERT(hw_1px_out,==,sw_out.z,"row,col:("<<row<<","<<col<<")")

        }else{
          ap_uint<Z_SIZE> z;
          ap_uint<Y_SIZE> y;
          ap_uint<THETA_SIZE> theta_id;
          ap_uint<P_SIZE> p_id;
          ap_uint<1> last;
          (last,z,y,theta_id,p_id) = out_symbol_stream.read();
          ASSERT(z,==,sw_out.z,"row,col:("<<row<<","<<col<<")")
          ASSERT(y,==,sw_out.y,"row,col:("<<row<<","<<col<<")")
          ASSERT(p_id,==,sw_out.p_id,"row,col:("<<row<<","<<col<<")")
          ASSERT(theta_id,==,sw_out.theta_id,"row,col:("<<row<<","<<col<<")")
          ASSERT(last,==,golden_last,"row,col:("<<row<<","<<col<<")")

          // calc theta_id
          // auto hw_theta_id = sw_impl::get_theta_idx(cnt, hw_out.St());
        }
      }
    }

    ASSERT(in_px_stream.size(),==,0)
    ASSERT(out_symbol_stream.size(),==,0)

    cout<<"\t|Success "<<endl;
  } // for test_idx
  return 0;
}