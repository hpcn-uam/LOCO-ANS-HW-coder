
# For arg parsing to work Vitis HLS needs to be called as: vitis_hls this_script.tcl [args]
set arglen [llength $argv]
set arg_idx 0

#set default parameters
# mode param | 0: all steps | 1: just syn & export
set mode 0
set target_id 0

#mode
if { $arg_idx < $arglen } {
  set mode [lindex $argv $arg_idx]
  incr arg_idx
}

#target_id
if { $arg_idx < $arglen } {
  set target_id [lindex $argv $arg_idx]
  incr arg_idx
}

switch -exact -- $target_id {
  0 {
    set sol_name "pynq_z2"
    set part xc7z020clg484-1
    set period  8
  }
  1 {
    set sol_name "zcu104"
    set part xczu7ev-ffvc1156-2-e
    set period 4
  }
  default {
    puts "Error: target id not supported"
    quit
  }
}

set ip_version $target_id

puts "Target : $sol_name"
puts "Period : $period"
puts "Part : $part"
puts "Mode : $mode"

open_project TSG_coder.hls_prj
set_top TSG_coder
add_files src/TSG_coder.cpp -cflags "-DTSG_CODER_TOP"
add_files ../input_buffers/src/input_buffers.cpp
add_files ../subsym_gen/src/subsym_gen.cpp
add_files ../ANS_coder/src/ANS_coder.cpp
add_files ../output_stack/src/output_stack.cpp
add_files -tb src/test.cpp
open_solution "${sol_name}" -flow_target vivado
set_part "$part"
create_clock -period ${period} -name default
config_compile -enable_auto_rewind=false
config_export -format ip_catalog -rtl verilog -library loco_ans -vendor HPCN -version "1.${ip_version}"

#source "./src/directives.tcl"
if { $mode == 0 } {
  csim_design -clean
}
csynth_design
if { $mode == 0 } {
  cosim_design -enable_dataflow_profiling -trace_level all
}
export_design -format ip_catalog
