/*==================================================================================================
*-----------------------------------------------------------------------------
*Copyright (c) 2018. All rights reserved
*-----------------------------------------------------------------------------
* @Filename      : test_odma.cpp
* @Author        : Tobias Alonso
* @Email         : talonsopugliese@gmail.com
* @Created On    : 2021-07-02 09:05:36
* @Description   :
*
*
*
* Last Modified : 2021-07-07 13:28:22 
*
* Revision      : 
*
* Disclaimer
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*==================================================================================================
*/

#include <iostream>
#include "dma.hpp"
#define NUM_OF_BLOCKS 2

#define OUT_BUFFER_SIZE  (MAX_ODMA_TRANSACTIONS*NUM_OF_BLOCKS)
using namespace std;
int main(int argc, char const *argv[])
{
  
  stream<odma_data> in_stream;
  stream<odma_command> in_cmds;
  odma_data hw_out[MAX_ODMA_TRANSACTIONS*2],golden_out[OUT_BUFFER_SIZE];
  ap_uint<NUM_OF_OUT_ELEM_BITS> block_size;
  ap_uint<DMA_ADDRESS_RANGE_BITS> offset;

  // gen input
  int acc_elements = 0;
  for(unsigned blk_id = 0; blk_id < NUM_OF_BLOCKS; ++blk_id) {
    block_size = MAX_ODMA_TRANSACTIONS-blk_id*10;
    offset = acc_elements;
    ap_uint<1> last_block = blk_id == NUM_OF_BLOCKS -1? 1:0;
    cout<<"Test "<<blk_id<<" | blk size: "<<block_size<<" | offset: "<<offset<<endl;

    for(unsigned i = 0; i < block_size; ++i) {
      odma_data in_elem = i*(blk_id+1);
      in_stream << in_elem;
      golden_out[i+offset] = in_elem;
    }

    in_cmds << (offset,block_size,last_block);

    acc_elements += block_size;
    ASSERT(acc_elements,<=,OUT_BUFFER_SIZE);
  }


  odma(in_cmds,in_stream,hw_out);

  //check
  cout<<"Checking blocks "<<endl;
  for(unsigned blk_id = 0; blk_id < NUM_OF_BLOCKS; ++blk_id) {
    for(unsigned i = offset; i < offset+block_size; ++i) {
      ASSERT(golden_out[i],==,hw_out[i],"Transaction: "<<i)
    }

    ASSERT(in_stream.empty())
    cout<<"Block "<<blk_id<<":  Pass "<<endl;
  }

  return 0;
}