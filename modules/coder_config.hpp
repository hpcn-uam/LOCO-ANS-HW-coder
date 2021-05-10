#ifndef CODER_CONFIG_HPP
#define CODER_CONFIG_HPP

// #include "coder.hpp"
// #define DEBUG_TYPES
#include <cstdint>
#include <assert.h>
#include "ap_int.h"

#ifndef __SYNTHESIS__
  #define ASSERT(x) assert(x)
#else
  #define ASSERT(x) 
#endif


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define AP_INT_MAX_W 16384

#define INPUT_BPP (8)

#define Z_SIZE (INPUT_BPP-1)
#define LOG2_Z_SIZE (3)
#define Y_SIZE (1)
#define THETA_SIZE (5) //32 tables
#define P_SIZE (5) //32 tables

#define BUFFER_ADDR_SIZE (11) //for 2048
#define CARD_BITS 5
#define ANS_SYMB_BITS CARD_BITS


#define CTX_ST_FINER_QUANT (false) 
#define CTX_ST_PRECISION (7) // number of fractional bits
#define MAX_ST_IDX  (14)//31//25 //(12)

#define CTX_NT_CENTERED_QUANT (true)
#define CTX_NT_PRECISION (6) // number of fractional bits
#define HALF_Y_CODER (false) 

// ANS coder 
#define ANS_MAX_SRC_CARDINALITY (9)
#define EE_MAX_ITERATIONS (7)
#define BUFFER_SIZE (2048)

#define NUM_ANS_THETA_MODES (15)//16 // supported theta_modes
#define NUM_ANS_P_MODES (32) //16 //32 // supported theta_modes
#define NUM_ANS_STATES (64)//32//64 // NUM_ANS_STATES only considers I range
#define ANS_I_RANGE_START NUM_ANS_STATES
#define NUM_ANS_BITS (6)
#define LOG2_ANS_BITS (3) // lg_2(NUM_ANS_BITS+1)

#define BIT_BLOCK_SIZE MAX(Z_SIZE,NUM_ANS_BITS+1)
#define LOG2_BIT_BLOCK_SIZE MAX(LOG2_Z_SIZE,LOG2_ANS_BITS)
static const ap_uint<CARD_BITS> tANS_cardinality_table[16] = { 1,1,1,1,1,2,2,4,4,8,8,8,8,8,8,0};
static const int32_t max_module_per_cardinality_table[16] = { 1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,2*EE_MAX_ITERATIONS,2*EE_MAX_ITERATIONS,4*EE_MAX_ITERATIONS,4*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,0*EE_MAX_ITERATIONS};


#endif // CODER_CONFIG_HPP