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

#include "TSG_coder.hpp"

// ap_axis adds tkeep and strobe signals, which I don't think I should be supporting 
// Instead, if a more AXIS compatible interface is needed, the next adapter 
// can be used
void TSG_input_adapter(
  stream<axis<ap_uint<IN_INTERF_WIDTH>,0,0,0>> &in,
  stream<coder_interf_t> &out){
  #ifdef TSG_INPUT_ADAPTER_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=in
  #pragma HLS INTERFACE axis register_mode=both register port=out
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif
  #pragma HLS PIPELINE style=flp

  START_SW_ONLY_LOOP(! in.empty())
  auto ie= in.read();
  out << bits_to_intf(ie.data,ie.last);
  END_SW_ONLY_LOOP
}


void output_data_interface(
  //inputs
  stream<byte_block<OUT_WORD_BYTES>> &byte_block_stream,
  //output
  stream<TSG_out_intf> &out_data_stream){
  #ifdef OUTPUT_DATA_INTERFACE_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=byte_block_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_data_stream
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif

  #pragma HLS PIPELINE style=flp

  auto in_data = byte_block_stream.read();

  //translate num_of_bytes to tkeep
  TSG_out_intf out_elem;
  out_elem.data = in_data.data;
  out_elem.keep = in_data.get_byte_mask();
  out_elem.strb = out_elem.keep;
  out_elem.last = in_data.is_last()? 1: 0;
  out_data_stream << out_elem;

}

void output_metadata_interface(
  //input
  stream<ap_uint<OUTPUT_STACK_BYTES_SIZE> > &last_byte_idx,
  stream<ap_uint<1>> &last_block,
  //output
  stream<tsg_blk_metadata> &out_blk_metadata){

  #ifdef OUTPUT_METADATA_INTERFACE_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=last_byte_idx
  #pragma HLS INTERFACE axis register_mode=both register port=last_block
  #pragma HLS INTERFACE axis register_mode=both register port=out_blk_metadata
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif

  #pragma HLS PIPELINE style=flp
  auto last_block_elem = last_block.read();
  auto last_byte_idx_elem = last_byte_idx.read();

  out_blk_metadata <<(last_byte_idx_elem,last_block_elem);
}


void TSG_coder(
  //input
  stream<coder_interf_t> &in,
  //output
  stream<TSG_out_intf> &byte_block_stream,
  stream<tsg_blk_metadata> &out_blk_metadata
  #if defined(EXTERNAL_ANS_ROM) && !defined(USE_TSG_INTERNAL_ROM)
  ,
  const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2],
  const tANS_table_t  tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]
  #endif
  //status registers
  // ap_uint<1> stack_overflow
  ){

  #ifdef TSG_CODER_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=in
  #pragma HLS INTERFACE axis register_mode=both register port=byte_block_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_blk_metadata
  #pragma HLS INTERFACE ap_ctrl_none port=return

    #ifdef EXTERNAL_ANS_ROM
    #pragma HLS INTERFACE mode=ap_memory  port=tANS_y_encode_table storage_type=rom_1p
    #pragma HLS INTERFACE mode=ap_memory  port=tANS_z_encode_table storage_type=rom_1p
    #endif

  #endif
  //status registers
  // #pragma HLS INTERFACE s_axilite port=stack_overflow bundle=control

  #pragma HLS DATAFLOW disable_start_propagation
  
  stream<coder_interf_t> inverted_data;
  #pragma HLS STREAM variable=inverted_data depth=2
  stream<ap_uint<1>> last_block;
  #pragma HLS bind_storage variable=last_block type=FIFO impl=LUTRAM
  #pragma HLS STREAM variable=last_block depth=32
  input_buffers(in, inverted_data,last_block);


  stream<subsymb_t> symbol_stream;
  #pragma HLS STREAM variable=symbol_stream depth=32
  #pragma HLS bind_storage variable=symbol_stream type=FIFO impl=LUTRAM
  START_SW_ONLY_LOOP(! inverted_data.empty())
  subsymbol_gen(inverted_data,symbol_stream);
  END_SW_ONLY_LOOP


  #ifdef USE_TSG_INTERNAL_ROM
    #pragma HLS DATAFLOW disable_start_propagation
     const tANS_table_t
      tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
      #include "../../ANS_tables/tANS_y_encoder_table.dat"
    };

     const tANS_table_t
      tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]{
      #include "../../ANS_tables/tANS_z_encoder_table.dat"
    };
  #endif

  stream<byte_block<OUT_WORD_BYTES>> coded_byte_stream;
  #pragma HLS STREAM variable=coded_byte_stream depth=2
  START_SW_ONLY_LOOP(!symbol_stream.empty())
  #ifdef EXTERNAL_ANS_ROM
    ANS_coder_ext_ROM(symbol_stream,coded_byte_stream,tANS_y_encode_table,tANS_z_encode_table);
  #else
    ANS_coder(symbol_stream,coded_byte_stream);
  #endif
  END_SW_ONLY_LOOP

  ap_uint<1> stack_overflow;
  stream<byte_block<OUT_WORD_BYTES>> inverted_byte_block;
  #pragma HLS STREAM variable=inverted_byte_block depth=2
  stream<ap_uint<OUTPUT_STACK_BYTES_SIZE>,2 > last_byte_idx;
  output_stack(coded_byte_stream,inverted_byte_block,last_byte_idx, stack_overflow);

  //Error handler: TODO
    // read "end of symbol stream", stack_overflow (other warning and error signals)
    // generate a single signal stating the war/error registars need to be checked
 
  //output interface
  START_SW_ONLY_LOOP(!inverted_byte_block.empty())
  output_data_interface(inverted_byte_block,byte_block_stream);
  END_SW_ONLY_LOOP

  START_SW_ONLY_LOOP(!last_byte_idx.empty() || !last_block.empty())
  ASSERT(last_byte_idx.empty(),==,last_block.empty())
  output_metadata_interface(last_byte_idx,last_block,out_blk_metadata);
  END_SW_ONLY_LOOP

  // pack_out_bytes(inverted_byte_block,byte_block_stream);
}


/*void TSG_coder_ext_ROM(
  //input
  stream<coder_interf_t> &in,
  //output
  stream<TSG_out_intf> &byte_block_stream,
  stream<tsg_blk_metadata> &out_blk_metadata,
  const tANS_table_t tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2],
  const tANS_table_t  tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]
  //status registers
  // ap_uint<1> stack_overflow
  ){

  #ifdef TSG_CODER_EXT_ROM_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=in
  #pragma HLS INTERFACE axis register_mode=both register port=byte_block_stream
  #pragma HLS INTERFACE axis register_mode=both register port=out_blk_metadata
  #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif
  //status registers
  // #pragma HLS INTERFACE s_axilite port=stack_overflow bundle=control

  #pragma HLS DATAFLOW disable_start_propagation
  
  stream<coder_interf_t> inverted_data;
  #pragma HLS STREAM variable=inverted_data depth=2
  stream<ap_uint<1>> last_block;
  #pragma HLS bind_storage variable=last_block type=FIFO impl=LUTRAM
  #pragma HLS STREAM variable=last_block depth=32
  input_buffers(in, inverted_data,last_block);


  stream<subsymb_t> symbol_stream;
  #pragma HLS STREAM variable=symbol_stream depth=32
  #pragma HLS bind_storage variable=symbol_stream type=FIFO impl=LUTRAM
  START_SW_ONLY_LOOP(! inverted_data.empty())
  subsymbol_gen(inverted_data,symbol_stream);
  END_SW_ONLY_LOOP

  stream<byte_block<OUT_WORD_BYTES>> coded_byte_stream;
  #pragma HLS STREAM variable=coded_byte_stream depth=2
  START_SW_ONLY_LOOP(!symbol_stream.empty())
  ANS_coder_ext_ROM(symbol_stream,coded_byte_stream,tANS_y_encode_table,tANS_z_encode_table);
  END_SW_ONLY_LOOP

  ap_uint<1> stack_overflow;
  stream<byte_block<OUT_WORD_BYTES>> inverted_byte_block;
  #pragma HLS STREAM variable=inverted_byte_block depth=2
  stream<ap_uint<OUTPUT_STACK_BYTES_SIZE>,2 > last_byte_idx;
  output_stack(coded_byte_stream,inverted_byte_block,last_byte_idx, stack_overflow);

  //Error handler: TODO
    // read "end of symbol stream", stack_overflow (other warning and error signals)
    // generate a single signal stating the war/error registars need to be checked
 
  //output interface
  START_SW_ONLY_LOOP(!inverted_byte_block.empty())
  output_data_interface(inverted_byte_block,byte_block_stream);
  END_SW_ONLY_LOOP

  START_SW_ONLY_LOOP(!last_byte_idx.empty() || !last_block.empty())
  ASSERT(last_byte_idx.empty(),==,last_block.empty())
  output_metadata_interface(last_byte_idx,last_block,out_blk_metadata);
  END_SW_ONLY_LOOP

  // pack_out_bytes(inverted_byte_block,byte_block_stream);
}
*/


void TSG_coder_double_lane(
  //Lane 0 
    //input
    stream<coder_interf_t> &in_0,
    //output
    stream<TSG_out_intf> &byte_block_stream_0,
    stream<tsg_blk_metadata> &out_blk_metadata_0,
  //Lane 1
    //input
    stream<coder_interf_t> &in_1,
    //output
    stream<TSG_out_intf> &byte_block_stream_1,
    stream<tsg_blk_metadata> &out_blk_metadata_1
  //status registers
  // ap_uint<1> stack_overflow
  ){

  #ifdef TSG_CODER_DOUBLE_LANE_TOP
  #pragma HLS INTERFACE axis register_mode=both register port=in_0
  #pragma HLS INTERFACE axis register_mode=both register port=byte_block_stream_0
  #pragma HLS INTERFACE axis register_mode=both register port=out_blk_metadata_0


  #pragma HLS INTERFACE axis register_mode=both register port=in_1
  #pragma HLS INTERFACE axis register_mode=both register port=byte_block_stream_1
  #pragma HLS INTERFACE axis register_mode=both register port=out_blk_metadata_1

  #pragma HLS INTERFACE ap_ctrl_none port=return
  #endif
  //status registers
  // #pragma HLS INTERFACE s_axilite port=stack_overflow bundle=control

  #pragma HLS DATAFLOW disable_start_propagation


  const tANS_table_t 
    tANS_y_encode_table[NUM_ANS_P_MODES][NUM_ANS_STATES][2]{
    #include "../../ANS_tables/tANS_y_encoder_table.dat"
  }; 

  const tANS_table_t 
    tANS_z_encode_table[NUM_ANS_THETA_MODES][NUM_ANS_STATES][Z_ANS_TABLE_CARDINALITY]{
    #include "../../ANS_tables/tANS_z_encoder_table.dat"
  }; 
  
  /***** LANE 0 *****/
  

  stream<coder_interf_t> inverted_data_0;
  #pragma HLS STREAM variable=inverted_data_0 depth=2
  stream<ap_uint<1>> last_block_0;
  #pragma HLS bind_storage variable=last_block_0 type=FIFO impl=LUTRAM
  #pragma HLS STREAM variable=last_block_0 depth=32
  input_buffers(in_0, inverted_data_0,last_block_0);


  static SubSymbolGenerator subsymbol_gen_0;
  #pragma HLS reset variable=subsymbol_gen_0
  stream<subsymb_t> symbol_stream_0;
  #pragma HLS STREAM variable=symbol_stream_0 depth=32
  #pragma HLS bind_storage variable=symbol_stream_0 type=FIFO impl=LUTRAM
  START_SW_ONLY_LOOP(! inverted_data_0.empty())
  subsymbol_gen_0.transform(inverted_data_0,symbol_stream_0);
  END_SW_ONLY_LOOP

  static ANSCoder<OUT_WORD_BYTES> ANS_coder_0;
  #pragma HLS reset variable=ANS_coder_0
  stream<byte_block<OUT_WORD_BYTES>> coded_byte_stream_0;
  #pragma HLS STREAM variable=coded_byte_stream_0 depth=2
  START_SW_ONLY_LOOP(!symbol_stream_0.empty())
  ANS_coder_0.code(symbol_stream_0,coded_byte_stream_0,tANS_y_encode_table,tANS_z_encode_table);
  END_SW_ONLY_LOOP

  ap_uint<1> stack_overflow_0;
  stream<byte_block<OUT_WORD_BYTES>> inverted_byte_block_0;
  #pragma HLS STREAM variable=inverted_byte_block_0 depth=2
  stream<ap_uint<OUTPUT_STACK_BYTES_SIZE>,2 > last_byte_idx_0;
  output_stack(coded_byte_stream_0,inverted_byte_block_0,last_byte_idx_0, stack_overflow_0);

  //Error handler: TODO
    // read "end of symbol stream", stack_overflow (other warning and error signals)
    // generate a single signal stating the war/error registars need to be checked
 
  //output interface
  START_SW_ONLY_LOOP(!inverted_byte_block_0.empty())
  output_data_interface(inverted_byte_block_0,byte_block_stream_0);
  END_SW_ONLY_LOOP

  START_SW_ONLY_LOOP(!last_byte_idx_0.empty() || !last_block_0.empty())
  ASSERT(last_byte_idx_0.empty(),==,last_block_0.empty())
  output_metadata_interface(last_byte_idx_0,last_block_0,out_blk_metadata_0);
  END_SW_ONLY_LOOP

    // pack_out_bytes(inverted_byte_block_0,byte_block_stream);


  /***** LANE 1 *****/


  stream<coder_interf_t> inverted_data_1;
  #pragma HLS STREAM variable=inverted_data_1 depth=2
  stream<ap_uint<1>> last_block_1;
  #pragma HLS bind_storage variable=last_block_1 type=FIFO impl=LUTRAM
  #pragma HLS STREAM variable=last_block_1 depth=32
  input_buffers(in_1, inverted_data_1,last_block_1);


  static SubSymbolGenerator subsymbol_gen_1;
  #pragma HLS reset variable=subsymbol_gen_1
  stream<subsymb_t> symbol_stream_1;
  #pragma HLS STREAM variable=symbol_stream_1 depth=32
  #pragma HLS bind_storage variable=symbol_stream_1 type=FIFO impl=LUTRAM
  START_SW_ONLY_LOOP(! inverted_data_1.empty())
  subsymbol_gen_1.transform(inverted_data_1,symbol_stream_1);
  END_SW_ONLY_LOOP

  static ANSCoder<OUT_WORD_BYTES> ANS_coder_1;
  #pragma HLS reset variable=ANS_coder_1
  stream<byte_block<OUT_WORD_BYTES>> coded_byte_stream_1;
  #pragma HLS STREAM variable=coded_byte_stream_1 depth=2
  START_SW_ONLY_LOOP(!symbol_stream_1.empty())
  ANS_coder_1.code(symbol_stream_1,coded_byte_stream_1,tANS_y_encode_table,tANS_z_encode_table);
  END_SW_ONLY_LOOP

  ap_uint<1> stack_overflow_1;
  stream<byte_block<OUT_WORD_BYTES>> inverted_byte_block_1;
  #pragma HLS STREAM variable=inverted_byte_block_1 depth=2
  stream<ap_uint<OUTPUT_STACK_BYTES_SIZE>,2 > last_byte_idx_1;
  output_stack(coded_byte_stream_1,inverted_byte_block_1,last_byte_idx_1, stack_overflow_1);

  //Error handler: TODO
    // read "end of symbol stream", stack_overflow (other warning and error signals)
    // generate a single signal stating the war/error registars need to be checked
 
  //output interface
  START_SW_ONLY_LOOP(!inverted_byte_block_1.empty())
  output_data_interface(inverted_byte_block_1,byte_block_stream_1);
  END_SW_ONLY_LOOP

  START_SW_ONLY_LOOP(!last_byte_idx_1.empty() || !last_block_1.empty())
  ASSERT(last_byte_idx_1.empty(),==,last_block_1.empty())
  output_metadata_interface(last_byte_idx_1,last_block_1,out_blk_metadata_1);
  END_SW_ONLY_LOOP

}