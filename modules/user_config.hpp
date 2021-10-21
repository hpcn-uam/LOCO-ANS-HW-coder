/*
 *
 *      Author: Tobías Alonso
 */

#ifndef USER_CONFIG_HPP
#define USER_CONFIG_HPP

#define INPUT_BPP (8)

//quantizers
#define CTX_ST_FINER_QUANT (false) 
#define CTX_ST_PRECISION (7) // number of fractional bits
#define MAX_ST_IDX  (14)//31//25 //(12)

#define CTX_NT_CENTERED_QUANT (true)
#define CTX_NT_PRECISION (6) // number of fractional bits
#define HALF_Y_CODER (true) 

// ANS coder 
constexpr int  ANS_MAX_SRC_CARDINALITY = 9;
constexpr int EE_MAX_ITERATIONS = 7;
constexpr int BUFFER_SIZE = 2048;

#define NUM_ANS_THETA_MODES (15) // supported theta_modes
#define NUM_ANS_P_MODES (32) // supported p_modes
#define NUM_ANS_STATES (64)// NUM_ANS_STATES only considers I range

#define P_SIZE (5)
#define THETA_SIZE (4)
#define NUM_ANS_BITS (6)
#define LOG2_ANS_BITS (3) // lg_2(NUM_ANS_BITS+1)

static const unsigned int tANS_cardinality_table[16] = {	1,1,1,1,1,2,2,4,4,8,8,8,8,8,8,0};
static const int max_module_per_cardinality_table[16] = {	1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,1*EE_MAX_ITERATIONS,2*EE_MAX_ITERATIONS,2*EE_MAX_ITERATIONS,4*EE_MAX_ITERATIONS,4*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,8*EE_MAX_ITERATIONS,0*EE_MAX_ITERATIONS};


#endif