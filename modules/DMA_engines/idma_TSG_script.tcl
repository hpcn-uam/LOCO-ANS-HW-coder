# ==================================================================================================
# -----------------------------------------------------------------------------
# Copyright (c) 2018. All rights reserved
# -----------------------------------------------------------------------------
#  @Filename      : idma_TSG_script.tcl
#  @Author        : Tobías Alonso
#  @Email         : tobi.alonso@gmail.com
#  @Created On    : 2021-07-02 11:10:04
#  @Description   :
#
#
#
#  Last Modified : 2021-07-27 11:40:04
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


set mode 1
set target_id 0


#mode
if { $arg_idx < $arglen } {
  set mode [lindex $argv $arg_idx]
  incr arg_idx
}
if { $mode != 1 } {
  puts "Mode must be 1 for this IP (Does not have testbench)"
  set mode 1
}
puts "Mode : $mode"


#target_id
if { $arg_idx < $arglen } {
  set target_id [lindex $argv $arg_idx]
  incr arg_idx
}

switch -exact -- $target_id {
  0 {
    set target_name "pynq_z2"
    set part xc7z020clg400-1
    set period  8
  }
  1 {
    set target_name "zcu104"
    set part xczu7ev-ffvc1156-2-e
    set period  3
  }
  default {
    puts "Error: target id not supported"
    quit
  }
}

set sol_name "solution_$target_id"
puts "Target : $target_name"
puts "Solution name : $sol_name"
puts "Period : $period"
puts "Part : $part"

#mode
# if { $arg_idx < $arglen } {
#   set mode [lindex $argv $arg_idx]
#   incr arg_idx
# }
# puts "Mode : $mode"

open_project idma_TSG.hls_prj
set_top idma_TSG
add_files src/dma.cpp
open_solution "${sol_name}" -flow_target vivado
set_part "$part"
create_clock -period ${period} -name default
config_compile -enable_auto_rewind=false
config_interface -m_axi_max_widen_bitwidth 64
config_interface -m_axi_alignment_byte_size 1
#source "./loopback_fifo.hls_prj/solution1/directives.tcl"
if { $mode == 0 } {
  csim_design -clean
}
csynth_design
if { $mode == 0 } {
  cosim_design -enable_dataflow_profiling -trace_level all
}
export_design -format ip_catalog -rtl verilog
