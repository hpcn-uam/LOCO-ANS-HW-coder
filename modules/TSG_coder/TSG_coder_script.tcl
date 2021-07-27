
# For arg parsing to work Vitis HLS needs to be called as: vitis_hls this_script.tcl [args]
set arglen [llength $argv]
set arg_idx 0

#set default parameters
# mode param | 0: all steps | 1: just syn & export
set mode 0 


#mode
if { $arg_idx < $arglen } {
  set mode [lindex $argv $arg_idx]
  incr arg_idx
}
puts "Mode : $mode" 

open_project TSG_coder.hls_prj
set_top TSG_coder
add_files src/TSG_coder.cpp -cflags "-DTSG_CODER_TOP"
add_files ../input_buffers/src/input_buffers.cpp
add_files ../subsym_gen/src/subsym_gen.cpp
add_files ../ANS_coder/src/ANS_coder.cpp
add_files ../output_stack/src/output_stack.cpp
add_files -tb src/test.cpp
open_solution "solution1" -flow_target vivado
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
config_compile -enable_auto_rewind=false
#source "./src/directives.tcl"
if { $mode == 0 } {
  csim_design -clean
}
csynth_design
if { $mode == 0 } {
  cosim_design -enable_dataflow_profiling -trace_level all
}
export_design -format ip_catalog