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
* @Filename      : test_idma.cpp
* @Author        : Tobias Alonso
* @Email         : talonsopugliese@gmail.com
* @Created On    : 2021-07-02 09:05:36
* @Description   :
*
*
*
* Last Modified : 2021-07-02 11:07:43 
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
  
  mem_data in[TB_MAX_BLOCK_SIZE];
  stream<mem_data> out_stream;

  for(unsigned test_id = 0; test_id < NUM_OF_TESTS; ++test_id) {
    ap_uint<NUM_OF_IN_ELEM_BITS> block_size = TB_MAX_BLOCK_SIZE-test_id*10;
    cout<<"Test "<<test_id<<" | blk size: "<<block_size<<endl;

    for(unsigned i = 0; i < block_size; ++i) {
      in[i]=i*(test_id+1);
    }

    idma(in,out_stream,block_size);

    //check
    for(unsigned i = 0; i < block_size; ++i) {
      mem_data out_elem = out_stream.read();
      ASSERT(in[i],==,out_elem,"Transaction: "<<i)
    }

    ASSERT(out_stream.empty())
    cout<<"     Pass "<<endl;
  }

  return 0;
}