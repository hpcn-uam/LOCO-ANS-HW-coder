
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

set module subsymbol_gen

open_project "subsym_gen.hls_prj"
set_top subsymbol_gen
add_files src/subsym_gen.cpp -cflags "-DSUBSYMBOL_GEN_TOP"
add_files -tb src/test.cpp -cflags "-Wno-unknown-pragmas -DSUBSYMBOL_GEN_TOP" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../input_buffers/src/input_buffers.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
open_solution "solution1" -flow_target vivado
set_part {xc7z020-clg484-1}
create_clock -period 10 -name default
config_compile -enable_auto_rewind=false
# config_compile -pipeline_style flp

# source "./directives.tcl"
if { $mode == 0 } {
  csim_design -clean
}
csynth_design
if { $mode == 0 } {
  cosim_design -enable_dataflow_profiling -trace_level all
}
export_design -format ip_catalog
