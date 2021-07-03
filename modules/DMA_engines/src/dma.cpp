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
  stream<ap_uint<DMA_ADDRESS_RANGE_BITS>> & offset,
  stream<ap_uint<NUM_OF_OUT_ELEM_BITS>> & num_of_elememts){

  #pragma HLS INTERFACE axis register_mode=both register port=in_stream
  //depth argument is only for tb purposes. is set to MAX_ODMA_TRANSACTIONS*2
  // given the tb parameters, writing outside of the MAX_ODMA_TRANSACTIONS*2
  //boundary will make the co-sim fail 
  #pragma HLS INTERFACE m_axi depth=MAX_ODMA_TRANSACTIONS*2 port=out offset=slave bundle=mem
  #pragma HLS INTERFACE s_axilite port=out bundle=control
  #pragma HLS INTERFACE axis register_mode=both register port=offset
  #pragma HLS INTERFACE axis register_mode=both register port=num_of_elememts
  #pragma HLS INTERFACE s_axilite port=return bundle=control

  ap_uint<DMA_ADDRESS_RANGE_BITS> _off = offset.read();
  ap_uint<DMA_ADDRESS_RANGE_BITS> _num_of_elememts = num_of_elememts.read();
  stream2mem(in_stream,out,_off,_num_of_elememts);
}


void loopback_fifo(
  stream<mem_data>  & in_stream,
  stream<mem_data> & out_stream,
  ap_uint<DMA_ADDRESS_RANGE_BITS> conf_offset,
  stream<ap_uint<DMA_ADDRESS_RANGE_BITS>> & out_offset,
  ap_uint<NUM_OF_IN_ELEM_BITS> conf_in_num_of_elememts,
  ap_uint<NUM_OF_OUT_ELEM_BITS> conf_out_num_of_elememts,
  stream<ap_uint<NUM_OF_OUT_ELEM_BITS>> & out_num_of_elememts){
  // #pragma HLS DATAFLOW
  #pragma HLS INTERFACE axis register_mode=both register port=in_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_stream
  #pragma HLS INTERFACE s_axilite port=conf_offset bundle=control
  #pragma HLS INTERFACE axis register_mode=both register port=out_offset
  #pragma HLS INTERFACE s_axilite port=conf_in_num_of_elememts bundle=control
  #pragma HLS INTERFACE s_axilite port=conf_out_num_of_elememts bundle=control
  #pragma HLS INTERFACE axis register_mode=both register port=out_num_of_elememts
  #pragma HLS INTERFACE s_axilite port=return bundle=control
  // #pragma HLS INTERFACE ap_ctrl_none port=return  

  stream<mem_data,MAX_ODMA_TRANSACTIONS> fifo;
  ap_uint<NUM_OF_OUT_ELEM_BITS> _out_num_of_elememts = 
    MIN(ap_uint<NUM_OF_OUT_ELEM_BITS>(conf_in_num_of_elememts),conf_out_num_of_elememts);


  for(ap_uint<NUM_OF_IN_ELEM_BITS> i = 0; i < conf_in_num_of_elememts; ++i) {
    mem_data in_elem = in_stream.read();
    if(i< _out_num_of_elememts) {
      fifo << in_elem;
    }
     
  }

  for(ap_uint<NUM_OF_IN_ELEM_BITS> i = 0; i < _out_num_of_elememts; ++i) {
    if(i==0) {
      out_offset << conf_offset;
      out_num_of_elememts << _out_num_of_elememts;
    }
    out_stream << fifo.read();
  }

}