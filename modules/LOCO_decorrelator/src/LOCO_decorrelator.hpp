
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
#define LOG2_MAX_COLS 13
constexpr int LOG2_MAX_ROWS = 16;
constexpr int MAX_COLS  = (1<<LOG2_MAX_COLS) ;
constexpr int MAX_ROWS  = (1<<LOG2_MAX_ROWS) ;

constexpr int BUFFER_COLS  = MAX_COLS ;
constexpr int BUFFER_ROWS = 1;



#define OPTIMIZED_POINTERS 1

#if OPTIMIZED_POINTERS
typedef ap_uint<LOG2_MAX_COLS> col_ptr_t;
typedef ap_uint<LOG2_MAX_ROWS> row_ptr_t;
typedef ap_uint<LOG2_MAX_ROWS+LOG2_MAX_COLS> pixel_ptr_t;
#else
typedef int col_ptr_t;
typedef int row_ptr_t;
typedef int pixel_ptr_t;
#endif


#define OPTIMIZED_PIXEL_TIPE 1
#if OPTIMIZED_PIXEL_TIPE
  typedef ap_int<10> err_t; // OPT can be reduced to 9
#else
  typedef int err_t; // OPT can be reduced to 9
#endif



#define CTX_B_PRECISION 3 // number of fractional bits

#define USING_DIV_RED_LUT 1
#define ERROR_REDUCTION 1
#define FORWARD_QUANTIZATION_ERROR 1


constexpr int CTX_0 = 0;
constexpr int CTX_BINS_PER_DIM =9;
constexpr int CTX_GRAD_BINS =(((CTX_BINS_PER_DIM)*(CTX_BINS_PER_DIM)*(CTX_BINS_PER_DIM)+1)/2);
constexpr int CTX_ADJUST_CNT_BITS   =6;
constexpr int CTX_ADJUST_CNT   =1<<CTX_ADJUST_CNT_BITS;

typedef  ap_int<8> ctx_bias_t;
#define CTX_CNT_SIZE  (CTX_ADJUST_CNT_BITS)  //OPT: CTX_ADJUST_CNT_BITS is actually enough
// #define CTX_CNT_SIZE  (CTX_ADJUST_CNT_BITS+1)  //OPT: CTX_ADJUST_CNT_BITS is actually enough
typedef  ap_uint<CTX_CNT_SIZE> ctx_cnt_t;
typedef  ap_int<1+CTX_ADJUST_CNT_BITS> ctx_acc_t;
// typedef  ap_int<3+CTX_ADJUST_CNT_BITS> ctx_acc_t;
// typedef  ap_int<INPUT_BPP+CTX_ADJUST_CNT_BITS> ctx_acc_t;

// OPT: ctx_Nt_t needs less than this actually
// it should store values in [-((cnt+1)>>1),((cnt)>>1) )
typedef  ap_int<CTX_CNT_SIZE+1+CTX_NT_PRECISION> ctx_Nt_t;
typedef  ap_uint<P_SIZE> ctx_p_idx_t;
#define CTX_ST_SIZE  (INPUT_BPP+CTX_ADJUST_CNT_BITS-1+CTX_ST_PRECISION)
typedef  ap_uint<CTX_ST_SIZE> ctx_St_t;



// create class to aggregate output. VITIS HLS aligns fields to 2^x, x>=3
class DecorrelatorOutput
{
  /*|St|cnt|p_idx|y|z|last|  */
  ap_uint< REM_REDUCT_SIZE
           +CTX_ST_SIZE
           +CTX_CNT_SIZE
           +P_SIZE
           +Y_SIZE
           +Z_SIZE
           +1 > data;
public:
  DecorrelatorOutput():data(0){};
  DecorrelatorOutput(
     unsigned int _remainder_reduct_bits ,unsigned int _St, unsigned int _cnt,
     unsigned int _p_idx, unsigned int _y, unsigned int _z, unsigned int _last){
      remainder_reduct_bits(_remainder_reduct_bits);
      St(_St);
      cnt(_cnt);
      p_idx(_p_idx);
      y(_y);
      z(_z);
      last(_last);
    };
  ~DecorrelatorOutput(){};

  inline void set_data(long unsigned int _data){ data=_data; };
  inline long unsigned int get_data(){return data;}

  inline void remainder_reduct_bits(unsigned int _remainder_reduct_bits){
    data(REM_REDUCT_SIZE+ Z_SIZE+2+P_SIZE + CTX_CNT_SIZE+ CTX_ST_SIZE-1,
          Z_SIZE+2+P_SIZE + CTX_CNT_SIZE+ CTX_ST_SIZE)=_remainder_reduct_bits; };
  inline unsigned int remainder_reduct_bits(){
    return data(REM_REDUCT_SIZE+ Z_SIZE+2+P_SIZE + CTX_CNT_SIZE+ CTX_ST_SIZE-1,
                Z_SIZE+2+P_SIZE + CTX_CNT_SIZE+ CTX_ST_SIZE);}

  inline void St(unsigned int _St){
    data(Z_SIZE+2+P_SIZE + CTX_CNT_SIZE+ CTX_ST_SIZE-1,Z_SIZE+2+P_SIZE + CTX_CNT_SIZE)=_St; };
  inline unsigned int St(){
    return data(Z_SIZE+2+P_SIZE + CTX_CNT_SIZE+ CTX_ST_SIZE-1,Z_SIZE+2+P_SIZE + CTX_CNT_SIZE);}

  inline void cnt(unsigned int _cnt){
    data(Z_SIZE+2+P_SIZE + CTX_CNT_SIZE -1,Z_SIZE+2+P_SIZE)=_cnt; };
  inline unsigned int cnt(){return data(Z_SIZE+2+P_SIZE + CTX_CNT_SIZE -1,Z_SIZE+2+P_SIZE);}

  inline void p_idx(unsigned int _p_idx){ data(Z_SIZE+2+P_SIZE-1,Z_SIZE+2)=_p_idx; };
  inline unsigned int p_idx(){return data(Z_SIZE+2+P_SIZE-1,Z_SIZE+2);}

  inline void y(unsigned int _y){ data[Z_SIZE+1]=_y; };
  inline unsigned int y(){return data[Z_SIZE+1];}

  inline void z(unsigned int _z){ data(Z_SIZE,1)=_z; };
  inline unsigned int z(){return data(Z_SIZE,1);}

  inline void last(unsigned int _last){ data[0]=_last; };
  inline unsigned int last(){return data[0];}

  DecorrelatorOutput & operator=(const DecorrelatorOutput & other){
    data = other.data;
    return *this;
  }

};


/*void LOCO_decorrelator(
  col_ptr_t config_cols,
  row_ptr_t config_rows,
  hls::stream<px_t>& src,
  hls::stream<px_t>& first_px_out,
  hls::stream<coder_interf_t> & out_symbols);*/

void LOCO_decorrelator(
  col_ptr_t config_cols,
  row_ptr_t config_rows,
  ap_uint<8> config_near,
  hls::stream<px_t>& src,
  hls::stream<px_t>& first_px_out,
  hls::stream<DecorrelatorOutput> & symbols,
  //READ-ONLY (from the user perspective) AXI-LITE registers
  ap_uint<8> &param_max_near,
  ap_uint<8> &param_num_of_tiles);


void LOCO_decorrelator_LS(
  col_ptr_t config_cols,
  row_ptr_t config_rows,
  hls::stream<px_t>& src,
  hls::stream<px_t>& first_px_out,
  hls::stream<DecorrelatorOutput> & symbols);

void St_idx_compute(
  hls::stream<DecorrelatorOutput> & in_symbols,
  hls::stream<coder_interf_t> & out_symbols);



#endif // LOCO_DECORRELATOR_HPP
