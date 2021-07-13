

#include <stdio.h>
#include "stdlib.h"
#include "xil_printf.h"
#include "xil_cache.h"
#include "inttypes.h"
// #include "ap_int.h"


#define INPUT_BLOCK_SIZE (2048*7-1)
#define OUTPUT_BLOCK_SIZE (INPUT_BLOCK_SIZE*2)
#define BAREMETAL 1


#if BAREMETAL
#undef __linux__
#endif

#include "xidma.h"
#include "xodma.h"
// #include "../../../../modules/coder_config.hpp"

#define NUM_ANS_THETA_MODES (15)//16 // supported theta_modes
#define NUM_ANS_P_MODES (32) //16 //32 // supported theta_modes

#define INPUT_BPP (8)
#define Z_SIZE (INPUT_BPP-1)
#define LOG2_Z_SIZE (3)
#define Y_SIZE (1)
#define THETA_SIZE (5) //32 tables
#define P_SIZE (5) //32 tables

#define ASSERT(v1,comp,v2,i) \
  if(!(v1 comp v2)){ \
  printf("%d| Test (" #v1 " " #comp " " #v2 ") Failed | " #v1 " = %ld" " | " \
   #v2 " = %ld" "\n",i,(long int)(v1),(long int)(v2)  ); \
  }


// DMA instance
XIdma idma;
XOdma odma;

volatile uint8_t out[OUTPUT_BLOCK_SIZE];
volatile uint32_t in[INPUT_BLOCK_SIZE];

#define WITH_ACM 1

#define GET_MASK(x) ((1<<x)-1) 

int main()
{

  printf("################################# \n\r");
  printf("Start Application  \n\r");

  //config idma
  int idma_status = XIdma_Initialize(&idma,XPAR_IDMA_0_DEVICE_ID);

  if(idma_status != XST_SUCCESS){
    printf("Error initializing IDMA\n\r");
    return XST_FAILURE;
  }

  //config odma
  // odma_conf = Xodma_LookupConfig(XPAR_ODMA_VARSIZE_0_DEVICE_ID);
  int odma_status = XOdma_Initialize(&odma,XPAR_ODMA_0_DEVICE_ID);

  if(odma_status != XST_SUCCESS){
    printf("Error initializing ODMA\n\r");
    return XST_FAILURE;
  }


  printf("Set up args\n\r");
  //idma
  int block_size = INPUT_BLOCK_SIZE;
//  int out_num_of_elem = 10;
  XIdma_Set_in_r(&idma,  (u64 )(&in));
  XIdma_Set_num_of_elememts(&idma,  block_size);
  printf("block_size: %d \n\r",block_size);

  if(block_size<=0){
	  printf("block_size<=0\n\r");
	  return 0;
  }

  //odma
  XOdma_Set_out_r(&odma, (u64 )(&out));


  // ************
  // Generate input 
  // ************
  printf("Set up input block\n\r");
  int blk_idx =4;
  for (int i = 0; i < block_size; ++i){
    int last = (i == block_size-1)? 1:0 ;
    int val = i;
    // val = 0;
    int z = blk_idx <= 3? val&0x1:(blk_idx <= 5? val & 0xF : val & 0x7F) ;
    int y = val & 0x80?1:0 ; 
    int theta_id = i >= NUM_ANS_THETA_MODES?NUM_ANS_THETA_MODES-1: i ;
    // theta_id =10;
    int p_id = blk_idx/2 ;

    u32 dma_data = 0;
    int acc_bits = 0;
    dma_data = p_id & GET_MASK(P_SIZE);
    acc_bits +=P_SIZE;

    dma_data |= (theta_id & GET_MASK(THETA_SIZE))<<acc_bits;
    acc_bits +=THETA_SIZE;

    dma_data |= (y & GET_MASK(Y_SIZE))<<acc_bits;
    acc_bits +=Y_SIZE;

    dma_data |= (z & GET_MASK(Z_SIZE))<<acc_bits;
    acc_bits +=Z_SIZE;

    // last bit
    dma_data |= (last & GET_MASK(1))<<acc_bits;
    acc_bits +=1;

    in[i]=dma_data;

  }

  const int default_val = 0xFE;

  for(unsigned i = 0; i < OUTPUT_BLOCK_SIZE; ++i) {
    out[i]=default_val;
  }


  //Run DMAs
  #if !WITH_ACM
    printf("Flush caches \n\r");
    Xil_DCacheFlush();
  #endif

  //start the accelerators
   printf("Check idma is available\n\r");
   while (!XIdma_IsIdle(&idma)) ;
   printf("Check odma is available\n\r");
   while (!XOdma_IsIdle(&odma)) ;

  printf("Launch dma\n\r");

  XIdma_Start(&idma);
  XOdma_Start(&odma);

  #if !WITH_ACM
  Xil_DCacheInvalidateRange((unsigned int )out, sizeof(*out)*out_num_of_elem);
  #endif

  printf("Waiting odma is done\n\r");
  //wait till end
  while (!XOdma_IsReady(&odma)) ;

  printf("Checking output \n\r");

  printf("    Get output bytes :");
  int out_bytes=0;
  for(unsigned i = 0; i < 4 ; ++i) {
	  int aux = out[i];
	  out_bytes |=  aux<<(i*8);
  }
  printf(" %d \n\r",out_bytes);

  printf("    Data \n\r");

  if(out_bytes> 25){
	  for(unsigned i = 4; i < 10+4; ++i) {
	  	      printf("%d (%d): %02X\n", i,i-4, out[i]);
	  	    }
	  printf("...\n");
	  for(unsigned i = out_bytes-16+4; i < out_bytes +4 ; ++i) {
	      printf("%d (%d): %02X\n", i,i-4, out[i]);
	    }
  }else{
	  for(unsigned i = 4; i < out_bytes +4 ; ++i) {
	      printf("%d (%d): %02X\n", i,i-4, out[i]);
	    }
  }





  printf("End Application\n\r");

  return 0;
}
