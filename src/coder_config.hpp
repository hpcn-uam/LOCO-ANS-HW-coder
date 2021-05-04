#ifndef CODER_CONFIG_HPP
#define CODER_CONFIG_HPP

// #include "coder.hpp"
#include "ap_int.h"
#define AP_INT_MAX_W 16384

#define INPUT_BPP (8)

#define Z_SIZE (INPUT_BPP-1)
#define Y_SIZE (1)
#define THETA_SIZE (5) //32 tables
#define P_SIZE (5) //32 tables

#define BUFFER_SIZE (2048) //2048
#define EE_MAX_ITERATIONS (3)

static const uint tANS_cardinality_table[32] = { 16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16};
static const uint max_module_per_cardinality_table[32] = { 16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS,16*EE_MAX_ITERATIONS};


#endif // CODER_CONFIG_HPP