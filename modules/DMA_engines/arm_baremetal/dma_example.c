

#include <stdio.h>
#include "stdlib.h"
#include "xil_printf.h"
#include "xil_cache.h"

#define INPUT_BLOCK_SIZE (2048)
#define OUTPUT_BLOCK_SIZE (INPUT_BLOCK_SIZE)

#include "inttypes.h"
#include "xhls_dma.h"

#define ASSERT(v1,comp,v2,i) \
  if(!(v1 comp v2)){ \
  printf("%d| Test (" #v1 " " #comp " " #v2 ") Failed | " #v1 " = %ld" " | " \
   #v2 " = %ld" "\n",i,(long int)(v1),(long int)(v2)  ); \
  }


// DMA instance
XHls_dma Hls_dma;

volatile int64_t out[OUTPUT_BLOCK_SIZE];
volatile int64_t in[INPUT_BLOCK_SIZE];

#define WITH_ACM 1
int main()
{
  int dma_status;



    XHls_dma_Config *dma_conf;
    printf("################################# \n\r");
    printf("Start Application (Variable size 2)\n\r");


//    int64_t in[OUTPUT_BLOCK_SIZE];
//    int64_t *out;
//    out = (int64_t *) malloc(sizeof(int64_t)*INPUT_BLOCK_SIZE);
//    out = (int64_t *) malloc(sizeof(int64_t)*OUTPUT_BLOCK_SIZE);

    dma_conf = XHls_dma_LookupConfig(XPAR_HLS_DMA_0_DEVICE_ID);

    dma_status = XHls_dma_CfgInitialize(&Hls_dma,dma_conf);
  if(dma_status != XST_SUCCESS){
    print("Error initializing DMA\n\r");
    return XST_FAILURE;
  }


  printf("Set up input block\n\r");
  in[0]=OUTPUT_BLOCK_SIZE-4;
    for(unsigned i = 1; i < INPUT_BLOCK_SIZE; ++i) {
        in[i]=i;
        out[i]=0;
      }



    //Run DMA: hls_dma(in,out);
    // set in and out ptrs
    printf("Set up in address\n\r");
    XHls_dma_Set_in_r(&Hls_dma,  (u64 )(&in));
    printf("Set up out address\n\r");
    XHls_dma_Set_out_r(&Hls_dma,  (u64 ) (&out));

  #if !WITH_ACM
    printf("invalidate caches \n\r");
//    Xil_DCacheInvalidate();
    Xil_DCacheFlush();
#endif

    //start the accelerator
    printf("Launch dma\n\r");
    XHls_dma_Start(&Hls_dma);

    //wait till end
    //      while (!XHls_dma_IsReady(&Hls_dma)) ;
    while (!XHls_dma_IsDone(&Hls_dma)) ;

    printf("Checking output \n\r");

    int elem2write= in[0]+3;
    ASSERT(elem2write,==,out[0],-1)
    printf("Got header. Writen vals: %d\n\r",elem2write);
    for(unsigned i = 0; i < OUTPUT_BLOCK_SIZE-1; ++i) {
        if(i<elem2write) {
            /* code */
        ASSERT(in[i],==,out[i+1],i)
        }else{
        ASSERT(0,==,out[i+1],i);
        }
    }

    for(int i = INPUT_BLOCK_SIZE; i < INPUT_BLOCK_SIZE+2; i++){
      printf("out of bound %d: %d\n\r",i,in[i]);
    }

    printf("End Application\n\r");

    return 0;
}
