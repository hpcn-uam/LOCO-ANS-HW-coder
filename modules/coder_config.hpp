#ifndef CODER_CONFIG_HPP
#define CODER_CONFIG_HPP

#include <cstdint>
#include <assert.h>
#include "ap_int.h"
#include <cmath>

// HLS libs
#include <hls_stream.h>
#include <ap_axi_sdata.h>


/********************** Aux functions  ******************************/
#ifndef __SYNTHESIS__
  #define GET_ASSERT_MACRO(_1,_2,_3,_4,NAME,...) NAME
  #define ASSERT(...)  GET_ASSERT_MACRO(__VA_ARGS__,ASSERT4,ASSERT3,ASSERT2,ASSERT1)(__VA_ARGS__)
  #define ASSERT1(x) assert(x);
  // #define ASSERT2(x) assert(x) undefined
  #define ASSERT3(v1,comp,v2) \
  if(!(v1 comp v2)){ \
  std::cout<<"Test (" #v1 " " #comp " " #v2 ") Failed | " #v1 " = " <<v1 << " | " \
  << #v2 " = "<<v2 <<std::endl; \
  assert(v1 comp v2); \
  }

  #define ASSERT4(v1,comp,v2,text) \
  if(!(v1 comp v2)){ \
  std::cout<<text<< "| Test (" #v1 " " #comp " " #v2 ") Failed | " #v1 " = " <<v1 << " | " \
  << #v2 " = "<<v2 <<std::endl; \
  assert(v1 comp v2); \
  }

  #include <iostream>
  using namespace std;

  #define START_SW_ONLY_LOOP(cont_condition) while (cont_condition){
  #define END_SW_ONLY_LOOP }
#else
  #define START_SW_ONLY_LOOP(cont_condition)
  #define END_SW_ONLY_LOOP
  #define ASSERT(...)
#endif

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

constexpr int floorlog2(int x){
  return  x == 0 ? -1 : x == 1 ? 0 : 1+floorlog2(x >> 1);
}

constexpr int ceillog2(int x){
  return x == 0 ? -1 :x == 1 ? 0 : floorlog2(x - 1) + 1;
}

#define NBITS2(n) ((n&2)?1:0)
#define NBITS4(n) ((n&(0xC))?(2+NBITS2(n>>2)):(NBITS2(n)))
#define NBITS8(n) ((n&0xF0)?(4+NBITS4(n>>4)):(NBITS4(n)))
#define NBITS16(n) ((n&0xFF00)?(8+NBITS8(n>>8)):(NBITS8(n)))
#define NBITS32(n) ((n&0xFFFF0000)?(16+NBITS16(n>>16)):(NBITS16(n)))
#define NBITS(n) (n==0?0:NBITS32(n)+1)



/********************** General parameters ******************************/

//!!!!!! px is defined as unsigned char (ap_uint<>) generates some undesired effects
#define INPUT_BPP (8)
typedef ap_uint<INPUT_BPP> px_t;
// typedef unsigned char px_t;

constexpr int MAXVAL = (1<<INPUT_BPP)-1;
#define Z_SIZE (INPUT_BPP-1)
#define LOG2_Z_SIZE NBITS(Z_SIZE)// (3) // TODO: test  NBITS(Z_SIZE)
#define Y_SIZE (1)
#define THETA_SIZE (5) //32 tables
#define P_SIZE (5) //32 tables

constexpr int BUFFER_SIZE = 2048;
constexpr int BUFFER_ADDR_SIZE =  ceillog2(BUFFER_SIZE);
constexpr int ANS_SYMB_BITS = 5;
constexpr int CARD_BITS = ANS_SYMB_BITS;


#define CTX_ST_FINER_QUANT (false)
#define CTX_ST_PRECISION (7) // number of fractional bits
#define MAX_ST_IDX  (14)//31//25 //(12)

#define CTX_NT_CENTERED_QUANT (true)
#define CTX_NT_PRECISION (6) // number of fractional bits
#define HALF_Y_CODER (true)
#define CTX_NT_HALF_IDX (1<<(CTX_NT_PRECISION-1))
#define CTX_NT_QUANT_BINS (1<<(CTX_NT_PRECISION))


#define ITERATIVE_ST 1
/********************** ANS coder ******************************/

// Coder input interface types and conversions
#define REM_REDUCT_SIZE NBITS(Z_SIZE)
#define PRED_SYMB_SIZE ( Z_SIZE + Y_SIZE + THETA_SIZE + P_SIZE)
#define SYMB_DATA_SIZE PRED_SYMB_SIZE
#define SYMB_CTRL_SIZE (1+REM_REDUCT_SIZE)

typedef ap_uint<SYMB_DATA_SIZE> symb_data_t;
typedef ap_uint<SYMB_CTRL_SIZE> symb_ctrl_t;
typedef ap_uint<SYMB_DATA_SIZE+SYMB_CTRL_SIZE> coder_interf_t; //


#define SYMBOL_ENDIANNESS_LITTLE true
constexpr int EE_REMAINDER_SIZE = Z_SIZE;
constexpr int ANS_MAX_SRC_CARDINALITY =9;
constexpr int ANS_SUBSYMBOL_BITS =  ceillog2(ANS_MAX_SRC_CARDINALITY) ;
constexpr int Z_ANS_TABLE_CARDINALITY =(1<<ANS_SUBSYMBOL_BITS); // power of 2  equal or just above  ANS_MAX_SRC_CARDINALITY
constexpr int EE_MAX_ITERATIONS = 7;
constexpr int INITIAL_ANS_STATE = 0;

#define NUM_ANS_THETA_MODES (15)//16 // supported theta_modes
#define NUM_ANS_P_MODES (32) //16 //32 // supported theta_modes
#define NUM_ANS_STATES (64)//32//64 // NUM_ANS_STATES only considers I range
#define ANS_I_RANGE_START NUM_ANS_STATES
#define NUM_ANS_BITS (6)
#define LOG2_ANS_BITS (3) // lg_2(NUM_ANS_BITS+1)

#define BIT_BLOCK_SIZE MAX(Z_SIZE,NUM_ANS_BITS+1)
#define LOG2_BIT_BLOCK_SIZE MAX(LOG2_Z_SIZE,LOG2_ANS_BITS)
static const ap_uint<CARD_BITS> tANS_cardinality_table[16] = { 1,1,1,1,1,2,2,4,4,8,8,8,8,8,8,0};
//todo ensure to clamp values
static const ap_uint<Z_SIZE> max_module_per_cardinality_table[16] = { 1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,2*EE_MAX_ITERATIONS,2*EE_MAX_ITERATIONS,4*EE_MAX_ITERATIONS,4*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,0*EE_MAX_ITERATIONS};


#if SYMBOL_ENDIANNESS_LITTLE
  // +1 cause I need to send the bit marker
  // +7 to compute obtain ceil instead of floor
  constexpr int  OUT_WORD_BYTES = ((NUM_ANS_BITS+1+7)/8);
#else
  constexpr int  OUT_WORD_BYTES = 1;
#endif
constexpr int LOG2_OUT_WORD_BYTES = ceillog2(OUT_WORD_BYTES);
constexpr int LOG2_OUTPUT_SIZE = (LOG2_OUT_WORD_BYTES+3);
constexpr int OUTPUT_SIZE = (8*OUT_WORD_BYTES);
constexpr int MAX_SUPPORTED_BPP = 16;

// force OUTPUT_STACK_SIZE has to be 2^int
// Why to do this:
// * BRAMS are going to have 2^int addresses anyway. BRAM availability determines
//     how big we can set OUTPUT_STACK_SIZE
constexpr int  OUTPUT_STACK_SIZE =
    (1<< ceillog2(int(BUFFER_SIZE*MAX_SUPPORTED_BPP/OUTPUT_SIZE)) );

constexpr int OUTPUT_STACK_ADDRESS_SIZE = ceillog2(OUTPUT_STACK_SIZE);
constexpr int OUTPUT_STACK_BYTES_SIZE = ceillog2(OUTPUT_STACK_SIZE*OUT_WORD_BYTES);

// constexpr int OUTPUT_STACK_ADDRESS_SIZE = int(log2(OUTPUT_STACK_SIZE));
// #define OUTPUT_STACK_ADDRESS_SIZE int(log2(OUTPUT_STACK_SIZE))



/********************** TSG output interface ******************************/
typedef ap_uint<OUTPUT_STACK_BYTES_SIZE+1> tsg_blk_metadata;
typedef hls::axis<ap_uint<OUTPUT_SIZE>,0,0,0 > TSG_out_intf;

/********************** memory interface ******************************/


constexpr int IN_DMA_BYTES = 1;
constexpr int OUT_DMA_BYTES = 1;

constexpr int IN_INTERF_WIDTH = IN_DMA_BYTES*8;
constexpr int OUT_INTERF_WIDTH = OUT_DMA_BYTES*8;
constexpr int TB_MAX_BLOCK_SIZE = 2048;
typedef ap_uint<IN_INTERF_WIDTH> idma_data;
typedef ap_uint<OUT_INTERF_WIDTH> odma_data;

constexpr int MAX_ODMA_TRANSACTIONS =
        int((OUTPUT_STACK_SIZE*OUT_WORD_BYTES+OUT_DMA_BYTES-1)/OUT_DMA_BYTES);

constexpr int NUM_OF_IN_ELEM_BITS = 32;
constexpr int NUM_OF_OUT_ELEM_BITS = ceillog2(MAX_ODMA_TRANSACTIONS+1);


constexpr int DMA_ADDRESS_RANGE_BITS = 32; // this limits the maximum file size

#endif // CODER_CONFIG_HPP
