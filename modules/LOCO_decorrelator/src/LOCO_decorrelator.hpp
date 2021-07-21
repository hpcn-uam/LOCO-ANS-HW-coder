
#ifndef LOCO_DECORRELATOR_HPP
#define LOCO_DECORRELATOR_HPP


#include <hls_stream.h>
#include <ap_int.h>
#include <cmath>
#include <algorithm>

#include "../../coder_config.hpp"

#define C_DEBUG 

#ifndef __SYNTHESIS__
  #ifdef C_DEBUG
    #define DEBUG
  #endif
#endif

#ifdef DEBUG 
#include "sw_implementation.hpp"
#endif

//#include <ap_fixed.h>
//#include <hls_opencv.h>

// if MAX_ROWS is a  power of 2 logic is simplified 
// not a big deal if this is changed
#define LOG2_MAX_COLS 10
constexpr int LOG2_MAX_ROWS = 16;
constexpr int MAX_COLS  = (1<<LOG2_MAX_COLS) ;
constexpr int MAX_ROWS  = (1<<LOG2_MAX_ROWS) ;

constexpr int BUFFER_COLS  = MAX_COLS ;
constexpr int BUFFER_ROWS = 1;



#define OPTIMIZED_POINTERS 1

#if OPTIMIZED_POINTERS
typedef ap_uint<LOG2_MAX_COLS> col_ptr_t;
typedef ap_uint<LOG2_MAX_ROWS> row_ptr_t;
#else
typedef int col_ptr_t;
typedef int row_ptr_t;
#endif


#define OPTIMIZED_PIXEL_TIPE 1
#if OPTIMIZED_PIXEL_TIPE
  typedef unsigned char px_t;
  // typedef  ap_uint<8> px_t;
  typedef ap_int<10> err_t; // OPT can be reduced to 9
#else
  // typedef unsigned int px_t;
  typedef unsigned char px_t;
  typedef int err_t; // OPT can be reduced to 9
#endif



#define CTX_B_PRECISION 3 // number of fractional bits

#define END_OF_LINE_CALL 0

void LOCO_decorrelator(
  col_ptr_t config_cols,
  row_ptr_t config_rows,
  hls::stream<px_t>& src,
  hls::stream<err_t> & symbols);





#endif // LOCO_DECORRELATOR_HPP