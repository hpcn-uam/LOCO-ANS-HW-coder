

#include <stdio.h>
#include "stdlib.h"
#include "xil_printf.h"
#include "xil_cache.h"

#define INPUT_BLOCK_SIZE (256)
#define OUTPUT_BLOCK_SIZE (INPUT_BLOCK_SIZE*2)

#include "inttypes.h"
#include "xidma.h"
#include "xodma_varsize.h"
#include "xloopback_fifo.h"

#define ASSERT(v1,comp,v2,i) \
  if(!(v1 comp v2)){ \
  printf("%d| Test (" #v1 " " #comp " " #v2 ") Failed | " #v1 " = %ld" " | " \
   #v2 " = %ld" "\n",i,(long int)(v1),(long int)(v2)  ); \
  }


// DMA instance
XIdma idma;
XLoopback_fifo loopback_fifo;
XOdma_varsize odma;

volatile int32_t out[OUTPUT_BLOCK_SIZE];
volatile int32_t in[INPUT_BLOCK_SIZE];

#define WITH_ACM 1
int main()
{

  int idma_status,lb_fifo_status,odma_status;
  XLoopback_fifo_Config *loopback_fifo_conf;
  XIdma_Config *idma_conf;
  XOdma_varsize_Config *odma_conf;
  printf("################################# \n\r");
  printf("Start Application (v2) \n\r");

  //config idma
  idma_conf = XIdma_LookupConfig(XPAR_IDMA_0_DEVICE_ID);
  idma_status = XIdma_CfgInitialize(&idma,idma_conf);

  if(idma_status != XST_SUCCESS){
    printf("Error initializing IDMA\n\r");
    return XST_FAILURE;
  }

  //config LOOPBACK_FIFO
  loopback_fifo_conf = XLoopback_fifo_LookupConfig(XPAR_LOOPBACK_FIFO_0_DEVICE_ID);
  lb_fifo_status = XLoopback_fifo_CfgInitialize(&loopback_fifo,loopback_fifo_conf);

  if(lb_fifo_status != XST_SUCCESS){
    printf("Error initializing LOOPBACK_FIFO\n\r");
    return XST_FAILURE;
  }

  //config odma
  // odma_conf = Xodma_LookupConfig(XPAR_ODMA_VARSIZE_0_DEVICE_ID);
  odma_status = XOdma_varsize_Initialize(&odma,XPAR_ODMA_VARSIZE_0_DEVICE_ID);

  if(odma_status != XST_SUCCESS){
    printf("Error initializing ODMA\n\r");
    return XST_FAILURE;
  }


  printf("Set up args\n\r");
  //idma
  int block_size = INPUT_BLOCK_SIZE;
  XIdma_Set_in_r(&idma,  (u64 )(&in));
  XIdma_Set_num_of_elememts(&idma,  block_size);
  printf("block_size: %d \n\r",block_size);

  //lb fifo
  int offset= 20;
  ASSERT(offset, <,INPUT_BLOCK_SIZE,0);
  int out_num_of_elem = block_size-50;
  out_num_of_elem = out_num_of_elem <0? 0 : out_num_of_elem;

  XLoopback_fifo_Set_conf_offset(&loopback_fifo, offset);
  XLoopback_fifo_Set_conf_in_num_of_elememts(&loopback_fifo, block_size);
  XLoopback_fifo_Set_conf_out_num_of_elememts(&loopback_fifo, out_num_of_elem);
  printf("offset: %d \n\r",offset);
  printf("out_num_of_elem: %d \n\r",out_num_of_elem);

  //odma
  XOdma_varsize_Set_out_r(&odma, (u64 )(&out));

  printf("Set up input block\n\r");
  const int default_val = 0xCAFFEE;
  for(unsigned i = 0; i < block_size; ++i) {
    in[i]=i;
  }
  for(unsigned i = 0; i < OUTPUT_BLOCK_SIZE; ++i) {
    out[i]=default_val;
  }


  //Run DMAs
  Xil_DCacheFlush();
  #if !WITH_ACM
    printf("Flush caches \n\r");
    Xil_DCacheFlush();
  #endif

  //start the accelerators
   printf("Check idma is available\n\r");
   while (!XIdma_IsIdle(&idma)) ;
   printf("Check loopback_fifo is available\n\r");
   while (!XLoopback_fifo_IsIdle(&loopback_fifo)) ;
   printf("Check odma is available\n\r");
   while (!XOdma_varsize_IsIdle(&odma)) ;

  printf("Launch dma\n\r");

  XIdma_Start(&idma);
  XLoopback_fifo_Start(&loopback_fifo);
  XOdma_varsize_Start(&odma);


  Xil_DCacheInvalidateRange((unsigned int )out, sizeof(*out)*out_num_of_elem);

  printf("Waiting odma is done\n\r");
  //wait till end
  while (!XOdma_varsize_IsIdle(&odma)) ;

  printf("Checking output \n\r");

  //before offset
  printf("    Before offset \n\r");
  for(unsigned i = 0; i < offset; ++i) {
    ASSERT(default_val,==,out[i],i);
  }

  printf("    Data \n\r");
  for(unsigned i = 0; i < out_num_of_elem ; ++i) {
    ASSERT(in[i],==,out[i+offset],i)
  }

  printf("  After data \n\r");
  for(unsigned i = out_num_of_elem+offset; i < OUTPUT_BLOCK_SIZE; ++i) {
    ASSERT(default_val,==,out[i],i);
  }

  printf("End Application\n\r");

  return 0;
}
