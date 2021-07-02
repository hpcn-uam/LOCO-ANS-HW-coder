/*==================================================================================================
*-----------------------------------------------------------------------------
*Copyright (c) %Y. All rights reserved
*-----------------------------------------------------------------------------
* @Filename      : dma.cpp
* @Author        : Tobías Alonso
* @Email         : talonsopugliese@gmail.com
* @Created On    : 2021-07-02 09:27:01
* @Description   :
*
*
*
* Last Modified : 2021-07-02 11:01:58 
*
* Revision      : 
*
* Disclaimer
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*==================================================================================================
*/


#include "dma.hpp"

using namespace hls;



void idma(
  volatile mem_data *in,
  stream<mem_data> & out_stream,
  ap_uint<NUM_OF_IN_ELEM_BITS> num_of_elememts){

  #pragma HLS INTERFACE m_axi depth=TB_MAX_BLOCK_SIZE port=in offset=slave bundle=mem
  #pragma HLS INTERFACE axis register_mode=both register port=out_stream
  #pragma HLS INTERFACE s_axilite port=in bundle=control
  #pragma HLS INTERFACE s_axilite port=num_of_elememts bundle=control
  #pragma HLS INTERFACE s_axilite port=return bundle=control 
  mem2stream(in,out_stream,num_of_elememts);

}

void odma_VarSize(
  stream<mem_data>  & in_stream,
  volatile mem_data *out,
  stream<ap_uint<NUM_OF_OUT_ELEM_BITS>> & num_of_elememts){

  #pragma HLS INTERFACE axis register_mode=both register port=in_stream
  #pragma HLS INTERFACE m_axi depth=MAX_ODMA_TRANSACTIONS port=out offset=slave bundle=mem
  #pragma HLS INTERFACE s_axilite port=out bundle=control
  #pragma HLS INTERFACE axis register_mode=both register port=num_of_elememts
  #pragma HLS INTERFACE s_axilite port=return bundle=control

  ap_uint<NUM_OF_OUT_ELEM_BITS> nwrites= num_of_elememts.read();
  stream2mem(in_stream,out,nwrites);

}
