/*==================================================================================================
*-----------------------------------------------------------------------------
*Copyright (c) 2021. All rights reserved
*-----------------------------------------------------------------------------
* @Filename      : test.cpp
* @Author        : Tobias Alonso
* @Email         : talonsopugliese@gmail.com
* @Created On    : 2021-07-02 09:05:36
* @Description   :
*
*
*
* Last Modified : 2021-07-02 09:05:36 
*
* Revision      : 
*
* Disclaimer
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*==================================================================================================
*/

#include "../../coder_config.hpp"
#include <hls_stream.h>

constexpr int INTERFACE_WIDTH = OUT_DMA_BYTES*8;
constexpr int TB_MAX_BLOCK_SIZE = 2048;
typedef ap_uint<INTERFACE_WIDTH> mem_data;
constexpr int NUM_OF_IN_ELEM_BITS = 32;
constexpr int NUM_OF_OUT_ELEM_BITS = ceillog2(MAX_ODMA_TRANSACTIONS+1);

using namespace hls;

template<int DW,int NE_W>
void mem2stream(
  volatile ap_uint<DW> *in_mem,
  stream<ap_uint<DW>> & out_stream,
  ap_uint<NE_W> num_of_elememts){

  ASSERT(DW,>,0)
  ASSERT(NE_W,>,0)

  mem2stream_loop: for (ap_uint<NE_W> i = 0; i < num_of_elememts; i++) {
    #pragma HLS PIPELINE
     out_stream<<  ap_uint<DW>(in_mem[i]);
  }
}

template<int DW,int NE_W,int OFF_W>
void stream2mem(
  stream<ap_uint<DW>> & in_stream,
  volatile ap_uint<DW> *out_mem,
  ap_uint<OFF_W> offset,
  ap_uint<NE_W> num_of_elememts){

  ASSERT(DW,>,0)
  ASSERT(NE_W,>,0)
  ASSERT(OFF_W,>,0)

  stream2mem_loop: for (ap_uint<OFF_W> i = offset; i < offset+num_of_elememts; i++) {
    #pragma HLS PIPELINE
    out_mem[i]= in_stream.read();
  }
}


void idma(
  volatile mem_data *in,
  stream<mem_data> & out_stream,
  ap_uint<NUM_OF_IN_ELEM_BITS> num_of_elememts);

void odma_VarSize(
  stream<mem_data>  & in_stream,
  volatile mem_data *out,
  stream<ap_uint<DMA_ADDRESS_RANGE_BITS>> & offset,
  stream<ap_uint<NUM_OF_OUT_ELEM_BITS>> & num_of_elememts);


void loopback_fifo(
  stream<mem_data>  & in_stream,
  stream<mem_data> & out_stream,
  ap_uint<DMA_ADDRESS_RANGE_BITS> conf_offset,
  stream<ap_uint<DMA_ADDRESS_RANGE_BITS>> & out_offset,
  ap_uint<NUM_OF_IN_ELEM_BITS> conf_in_num_of_elememts,
  ap_uint<NUM_OF_OUT_ELEM_BITS> conf_out_num_of_elememts,
  stream<ap_uint<NUM_OF_OUT_ELEM_BITS>> & out_num_of_elememts);