
#include "../../coder_config.hpp"

struct tANS_table_t {
  ap_uint<NUM_ANS_BITS> state;
  ap_uint<LOG2_ANS_BITS> bits;
} ;

// TODO? HDL would probably be the way to go
// This allows me to skip the BRAM init file from the C++ tables
// 
constexpr int Z_MODE_ADDRESS_SIZE = ceillog2(NUM_ANS_THETA_MODES) ; 
constexpr int Z_ANS_ROM_ADDR_SIZE = Z_MODE_ADDRESS_SIZE +NUM_ANS_BITS+ANS_SUBSYMBOL_BITS;
void Z_ANS_ROM(
  ap_uint<Z_ANS_ROM_ADDR_SIZE> address_A, 
  ap_uint<LOG2_ANS_BITS+NUM_ANS_BITS>  & d_out_A,
  ap_uint<Z_ANS_ROM_ADDR_SIZE> address_B, 
  ap_uint<LOG2_ANS_BITS+NUM_ANS_BITS>  & d_out_B){
    
  #ifdef Z_ANS_ROM_TOP
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #pragma HLS INTERFACE mode=ap_none  port=address_A 
  #pragma HLS INTERFACE mode=ap_none  port=d_out_A 
  #pragma HLS INTERFACE mode=ap_none  port=address_B 
  #pragma HLS INTERFACE mode=ap_none  port=d_out_B 
  #endif

  #pragma HLS PIPELINE II=1
  static const tANS_table_t 
    tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]{
    #include "../tANS_z_encoder_table.dat"
  }; 

  #pragma HLS aggregate variable=tANS_z_encode_table
  auto mode_A = address_A(Z_MODE_ADDRESS_SIZE+NUM_ANS_BITS+ANS_SUBSYMBOL_BITS-1,
                      NUM_ANS_BITS+ANS_SUBSYMBOL_BITS);
  auto state_A = address_A(NUM_ANS_BITS+ANS_SUBSYMBOL_BITS-1,ANS_SUBSYMBOL_BITS);
  auto subsym_A = address_A(ANS_SUBSYMBOL_BITS-1,0);
  tANS_table_t out_A = tANS_z_encode_table[mode_A][state_A][subsym_A];
  d_out_A =(out_A.bits, out_A.state);


  //
  auto mode_B = address_B(Z_MODE_ADDRESS_SIZE+NUM_ANS_BITS+ANS_SUBSYMBOL_BITS-1,
                      NUM_ANS_BITS+ANS_SUBSYMBOL_BITS);
  auto state_B = address_B(NUM_ANS_BITS+ANS_SUBSYMBOL_BITS-1,ANS_SUBSYMBOL_BITS);
  auto subsym_B = address_B(ANS_SUBSYMBOL_BITS-1,0);
  tANS_table_t out_B = tANS_z_encode_table[mode_B][state_B][subsym_B];
  d_out_B =(out_B.bits, out_B.state);


}


constexpr int Y_MODE_ADDRESS_SIZE = ceillog2(NUM_ANS_P_MODES) ; 
constexpr int ANS_Y_SUBSYMBOL_BITS = 1 ; 
constexpr int Y_ANS_ROM_ADDR_SIZE = Y_MODE_ADDRESS_SIZE +NUM_ANS_BITS+ANS_Y_SUBSYMBOL_BITS;
void Y_ANS_ROM(

  ap_uint<Y_ANS_ROM_ADDR_SIZE> address_A, 
  ap_uint<LOG2_ANS_BITS+NUM_ANS_BITS>  & d_out_A,
  ap_uint<Y_ANS_ROM_ADDR_SIZE> address_B, 
  ap_uint<LOG2_ANS_BITS+NUM_ANS_BITS>  & d_out_B){
    
  #ifdef Y_ANS_ROM_TOP
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #pragma HLS INTERFACE mode=ap_none  port=address_A 
  #pragma HLS INTERFACE mode=ap_none  port=d_out_A 
  #pragma HLS INTERFACE mode=ap_none  port=address_B 
  #pragma HLS INTERFACE mode=ap_none  port=d_out_B 
  #endif

  #pragma HLS PIPELINE II=1
  static const tANS_table_t 
    tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
    #include "../tANS_y_encoder_table.dat"
  }; 


  #pragma HLS aggregate variable=tANS_y_encode_table
  auto mode_A = address_A(Y_MODE_ADDRESS_SIZE+NUM_ANS_BITS+ANS_Y_SUBSYMBOL_BITS-1,
                      NUM_ANS_BITS+ANS_Y_SUBSYMBOL_BITS);
  auto state_A = address_A(NUM_ANS_BITS+ANS_Y_SUBSYMBOL_BITS-1,ANS_Y_SUBSYMBOL_BITS);
  auto subsym_A = address_A(ANS_Y_SUBSYMBOL_BITS-1,0);
  tANS_table_t out_A = tANS_y_encode_table[mode_A][state_A][subsym_A];
  d_out_A =(out_A.bits, out_A.state);


  //
  auto mode_B = address_B(Y_MODE_ADDRESS_SIZE+NUM_ANS_BITS+ANS_Y_SUBSYMBOL_BITS-1,
                      NUM_ANS_BITS+ANS_Y_SUBSYMBOL_BITS);
  auto state_B = address_B(NUM_ANS_BITS+ANS_Y_SUBSYMBOL_BITS-1,ANS_Y_SUBSYMBOL_BITS);
  auto subsym_B = address_B(ANS_Y_SUBSYMBOL_BITS-1,0);
  tANS_table_t out_B = tANS_y_encode_table[mode_B][state_B][subsym_B];
  d_out_B =(out_B.bits, out_B.state);


}


/*  static const tANS_table_t 
    tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
    #include "../../ANS_tables/tANS_y_encoder_table.dat"
  }; */


