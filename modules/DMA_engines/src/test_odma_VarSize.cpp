/*==================================================================================================
*-----------------------------------------------------------------------------
*Copyright (c) 2018. All rights reserved
*-----------------------------------------------------------------------------
* @Filename      : test_odma_VarSize.cpp
* @Author        : Tobias Alonso
* @Email         : talonsopugliese@gmail.com
* @Created On    : 2021-07-02 09:05:36
* @Description   :
*
*
*
* Last Modified : 2021-07-02 11:28:39 
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
#define NUM_OF_TESTS 2

using namespace std;
int main(int argc, char const *argv[])
{
  
  stream<mem_data> in_stream;
  mem_data hw_out[MAX_ODMA_TRANSACTIONS],golden_out[MAX_ODMA_TRANSACTIONS];
  stream<ap_uint<NUM_OF_OUT_ELEM_BITS>> block_size_stream;
  ap_uint<NUM_OF_OUT_ELEM_BITS> block_size;

  for(unsigned test_id = 0; test_id < NUM_OF_TESTS; ++test_id) {
    block_size = MAX_ODMA_TRANSACTIONS-test_id*10;
    block_size_stream <<block_size;
    cout<<"Test "<<test_id<<" | blk size: "<<block_size<<endl;

    for(unsigned i = 0; i < block_size; ++i) {
      mem_data in_elem = i*(test_id+1);
      in_stream << in_elem;
      golden_out[i] = in_elem;
    }

    odma_VarSize(in_stream,hw_out,block_size_stream);

    //check
    for(unsigned i = 0; i < block_size; ++i) {
      ASSERT(golden_out[i],==,hw_out[i],"Transaction: "<<i)
    }

    ASSERT(in_stream.empty())
    ASSERT(block_size_stream.empty())
    cout<<"     Pass "<<endl;
  }

  return 0;
}