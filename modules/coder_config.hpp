#ifndef CODER_CONFIG_HPP
#define CODER_CONFIG_HPP

#include <cstdint>
#include <assert.h>
#include "ap_int.h"

#ifndef __SYNTHESIS__
  #define GET_ASSERT_MACRO(_1,_2,_3,_4,NAME,...) NAME
  #define ASSERT(...)  GET_ASSERT_MACRO(__VA_ARGS__,ASSERT4,ASSERT3,ASSERT2,ASSERT1)(__VA_ARGS__)
  #define ASSERT1(x) assert(x)
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
#else
  #define ASSERT(...) 
#endif

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

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
#define EE_REMAINDER_SIZE Z_SIZE
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
//todo ensure to clamp values
static const ap_uint<Z_SIZE> max_module_per_cardinality_table[16] = { 1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,2*EE_MAX_ITERATIONS,2*EE_MAX_ITERATIONS,4*EE_MAX_ITERATIONS,4*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,0*EE_MAX_ITERATIONS};


#endif // CODER_CONFIG_HPP