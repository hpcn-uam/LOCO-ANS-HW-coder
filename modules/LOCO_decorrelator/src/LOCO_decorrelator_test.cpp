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

// #define INPUT_IMG 
// #define WRITE_OUTPUT 


#ifdef INPUT_IMG
#include <opencv2/imgcodecs.hpp>
#endif

#ifdef WRITE_OUTPUT
#include <fstream>
#endif

using namespace std;
using namespace hls;
using namespace sw_impl;

// #define PRINT_INPUT

// #define ROWS 128
// #define COLS 128
#define ROWS 64
#define COLS 64

#ifdef INPUT_IMG
const int NUM_OF_TESTS = 1;
#else
const int NUM_OF_TESTS = 8;
#endif

int main(int argc, char const *argv[])
{
  #ifdef INPUT_IMG
    if(argc < 2) {
      cout<<"Error"<<endl;
    }

    cv::Mat img_orig = cv::imread( argv[1] ,cv::IMREAD_UNCHANGED);
    if(img_orig.empty()){
      std::cerr<<"Empty image file"<<std::endl;
      return 2;
    }
  #endif

  hls::stream<px_t> in_px_stream,out_first_px;
  
  hls::stream<coder_interf_t>  out_symbol_stream;

  hls::stream<int> sw_in_stream;
  stream<ee_symb_data> sw_out_symbol_stream;
  #ifdef LOCO_DECORRELATOR_LS_TOP
  for(int near = 0; near < 1; ++near) {
  #else
  for(int near = 0; near < 3; ++near) {
  #endif
    cout<<"###############"<<" Near "<<near<<"###############"<<endl;
    for(unsigned test_idx = 0; test_idx < NUM_OF_TESTS; ++test_idx) {
      int img_rows = ROWS+test_idx;
      img_rows= img_rows > MAX_ROWS? MAX_ROWS:img_rows;
      int img_cols = COLS - test_idx;
      img_cols= img_cols < 5? 5:img_cols;
      
      constexpr int INPUT_MASK = (1<< INPUT_BPP)-1;
      std::function<px_t(int,int)> px_gen;
      std::string test_name;

      #ifdef INPUT_IMG
        px_gen = [&](int row,int col) {return img_orig.at<unsigned char>(row,col);};
        test_name = "img path";
        img_rows = img_orig.rows;
        img_cols = img_orig.cols;

      #else
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

      #endif
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
      ap_uint<8> param_max_near;
      ap_uint<8> param_num_of_tiles;
      #if 1
        hls::stream<DecorrelatorOutput> out_presymbol_stream;
        #ifdef LOCO_DECORRELATOR_LS_TOP
          LOCO_decorrelator_LS(img_rows,img_cols, in_px_stream, out_first_px,out_presymbol_stream);
        #else
          LOCO_decorrelator(img_rows,img_cols,near, in_px_stream, out_first_px,out_presymbol_stream,
                            param_max_near,param_num_of_tiles);
        #endif

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


      #ifdef WRITE_OUTPUT
      std::ofstream hw_out_file("hw_out_file.dat");
      #endif

      for (int row = 0; row < img_rows; ++row){
        for (int col = 0; col < img_cols; ++col){

          auto sw_out = sw_out_symbol_stream.read();
          int golden_last = (row == (img_rows-1)) && (col == (img_cols-1))?1:0; 
          if(row == 0 && col == 0) {
            auto hw_1px_out = out_first_px.read();
            ASSERT(hw_1px_out,==,sw_out.z,"row,col:("<<row<<","<<col<<")")

          }else{
            ap_uint<REM_REDUCT_SIZE> remainder_reduct;
            ap_uint<Z_SIZE> z;
            ap_uint<Y_SIZE> y;
            ap_uint<THETA_SIZE> theta_id;
            ap_uint<P_SIZE> p_id;
            ap_uint<1> last;
            auto hw_out = out_symbol_stream.read();
            (last,remainder_reduct,z,y,theta_id,p_id) = hw_out;
            ASSERT(z,==,sw_out.z,"row,col:("<<row<<","<<col<<")")
            ASSERT(y,==,sw_out.y,"row,col:("<<row<<","<<col<<")")
            ASSERT(p_id,==,sw_out.p_id,"row,col:("<<row<<","<<col<<")")
            ASSERT(theta_id,==,sw_out.theta_id,"row,col:("<<row<<","<<col<<")")
            ASSERT(last,==,golden_last,"row,col:("<<row<<","<<col<<")")
            ASSERT(remainder_reduct,==,sw_out.remainder_reduct_bits,"row,col:("<<row<<","<<col<<")")

            // calc theta_id
            // auto hw_theta_id = sw_impl::get_theta_idx(cnt, hw_out.St());
            
            #ifdef WRITE_OUTPUT
            hw_out_file << ( long unsigned int )(hw_out)<<",\n";
            #endif
          }
        }

      }

      #ifdef WRITE_OUTPUT
      hw_out_file.close();
      #endif

      ASSERT(in_px_stream.size(),==,0)
      ASSERT(out_symbol_stream.size(),==,0)

      cout<<"\t|Success "<<endl;
    } // for test_idx
  }
  return 0;
}
