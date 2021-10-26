/*
  Copyright 2021 Tobías Alonso, Autonomous University of Madrid
  This file is part of LOCO-ANS HW encoder.
  LOCO-ANS HW encoder is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  LOCO-ANS HW encoder is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with LOCO-ANS HW encoder.  If not, see <https://www.gnu.org/licenses/>.
*/
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
  mem_data hw_out[MAX_ODMA_TRANSACTIONS*2],golden_out[MAX_ODMA_TRANSACTIONS*2];
  ap_uint<NUM_OF_OUT_ELEM_BITS> block_size;
  ap_uint<DMA_ADDRESS_RANGE_BITS> offset;

  for(unsigned test_id = 0; test_id < NUM_OF_TESTS; ++test_id) {
    block_size = MAX_ODMA_TRANSACTIONS-test_id*10;
    offset = test_id*20;
    cout<<"Test "<<test_id<<" | blk size: "<<block_size<<" | offset: "<<offset<<endl;

    for(unsigned i = 0; i < block_size; ++i) {
      mem_data in_elem = i*(test_id+1);
      in_stream << in_elem;
      golden_out[i+offset] = in_elem;
    }

    stream< decltype(offset) > off_stream;
    stream< decltype(block_size) > block_size_stream;
    off_stream << offset;
    block_size_stream << block_size;
    odma_VarSize(in_stream,hw_out,off_stream,block_size_stream);

    //check
    for(unsigned i = offset; i < offset+block_size; ++i) {
      ASSERT(golden_out[i],==,hw_out[i],"Transaction: "<<i)
    }

    ASSERT(in_stream.empty())
    cout<<"     Pass "<<endl;
  }

  return 0;
}