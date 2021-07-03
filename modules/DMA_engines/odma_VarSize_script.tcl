# ==================================================================================================
# -----------------------------------------------------------------------------
# Copyright (c) 2018. All rights reserved
# -----------------------------------------------------------------------------
#  @Filename      : odma_VarSize_script.tcl
#  @Author        : Tobías Alonso
#  @Email         : tobi.alonso@gmail.com
#  @Created On    : 2021-07-02 11:10:04
#  @Description   :
# 
# 
# 
#  Last Modified : 2021-07-02 11:12:47 
# 
#  Revision      : 
# 
#  
# ==================================================================================================

# For arg parsing to work Vitis HLS needs to be called as: vitis_hls this_script.tcl [args]
set arglen [llength $argv]
set arg_idx 0

#set default parameters
# mode param | 0: all steps | 1: just syn & export
set mode 0 
set part "xc7z020clg400-1" 


#mode
if { $arg_idx < $arglen } {
  set mode [lindex $argv $arg_idx]
  incr arg_idx
}
puts "Mode : $mode" 

if { $arg_idx < $arglen } {
  set part [lindex $argv $arg_idx]
  incr arg_idx
}
puts "Part : $part" 


open_project odma_VarSize.hls_prj
set_top odma_VarSize
add_files src/dma.cpp
add_files -tb src/test_odma_VarSize.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
open_solution "solution1" -flow_target vivado
set_part {xc7z020clg400-1}
create_clock -period 10 -name default
config_interface -m_axi_max_widen_bitwidth 64
config_interface -m_axi_alignment_byte_size 1
#source "./odma_VarSize.hls_prj/solution1/directives.tcl"
if { $mode == 0 } {
  csim_design -clean
}
csynth_design
if { $mode == 0 } {
  cosim_design -enable_dataflow_profiling -trace_level all
}
export_design -format ip_catalog
