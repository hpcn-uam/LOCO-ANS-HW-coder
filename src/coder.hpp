
#define BUFFER_SIZE (2048) //2048

#include <hls_stream.h>
#define AP_INT_MAX_W 16384
#include "ap_int.h"
#include "ap_axi_sdata.h"

#define INPUT_BPP (8)

#define Z_SIZE (INPUT_BPP-1)
#define Y_SIZE (1)
#define THETA_SIZE (5) //32 tables
#define P_SIZE (5) //32 tables
#define SYMB_DATA_SIZE MAX(INPUT_BPP,Z_SIZE + Y_SIZE + THETA_SIZE + P_SIZE)

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

using namespace hls;

// typedef int symb_data_t;
typedef ap_uint<SYMB_DATA_SIZE> symb_data_t;

typedef ap_uint<INPUT_BPP> pixel_t;
struct compress_symbol_t{
    ap_uint<Z_SIZE> z;
    ap_uint<Y_SIZE> y;
    ap_uint<THETA_SIZE> theta_id;
    ap_uint<P_SIZE> p_id;

    compress_symbol_t(uint _z,uint _y,uint _theta,uint _p):z(_z),y(_y),theta_id(_theta),p_id(_p){};
    compress_symbol_t(symb_data_t data):z(data(Z_SIZE-1,0)),
                                          y(data(Z_SIZE+Y_SIZE-1,Z_SIZE)),
                                          theta_id(data(Z_SIZE+Y_SIZE+THETA_SIZE-1,Z_SIZE+Y_SIZE)),
                                          p_id(data(Z_SIZE+Y_SIZE+THETA_SIZE+P_SIZE-1,
                                                            Z_SIZE+Y_SIZE+THETA_SIZE)){};
    symb_data_t to_intf(){
      symb_data_t data;
      data(Z_SIZE-1,0)=z;
      data(Z_SIZE+Y_SIZE-1,Z_SIZE)=y;
      data(Z_SIZE+Y_SIZE+THETA_SIZE-1,Z_SIZE+Y_SIZE) = theta_id;
      data(Z_SIZE+Y_SIZE+THETA_SIZE+P_SIZE-1,Z_SIZE+Y_SIZE+THETA_SIZE) = p_id;
      return data;
    }
  };



class coder_interf_t{
  ap_uint<SYMB_DATA_SIZE+1> bits;
public:
  coder_interf_t():bits(0){}
  coder_interf_t(symb_data_t data,ap_uint<1> first_px_flag){
    bits = (first_px_flag,data);
    // bits(SYMB_DATA_SIZE-1,0) = data;
    // bits(SYMB_DATA_SIZE,SYMB_DATA_SIZE)=first_px_flag;
  }
  symb_data_t  get_data(){ return bits(SYMB_DATA_SIZE-1,0);}
  ap_uint<1>  is_first_px(){ return bits.test(SYMB_DATA_SIZE);}

  void set_data(symb_data_t data){ bits(SYMB_DATA_SIZE-1,0) = data;}
  void set_first_px_flag(ap_uint<1> first_px_flag){ bits(SYMB_DATA_SIZE,SYMB_DATA_SIZE)=first_px_flag;}


};

#define TUSER 1 // 1 bit to indicate what member of the union 
#define TID 0 
#define IDEST 0

#define IS_COMPRESS_SYMB 0
#define IS_FIRST_PX 1
// struct symbol_info_t{
//   symb_data_t data;
//   ap_uint<1> user; 
// }__attribute__((aligned(4)));
// typedef axis<symb_data_t,TUSER,TID,IDEST > symbol_info_t;
void coder(stream<coder_interf_t > &in, stream<coder_interf_t > &out);


