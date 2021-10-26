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

#ifndef FILE_WRITER_HPP
#define FILE_WRITER_HPP

#include "../../coder_config.hpp"
#include "../../DMA_engines/src/dma.hpp"

using namespace hls;

constexpr unsigned int OFFSET_INIT = 5;

void File_writer(
  //inputs
  stream<TSG_out_intf> &in_byte_block_stream,
  stream<tsg_blk_metadata> &in_blk_metadata,
  stream<px_t> &first_px_stream,
  //outputs
  stream<odma_data>  & out_stream,
  stream<odma_command> & out_command);


#endif // FILE_WRITER_HPP