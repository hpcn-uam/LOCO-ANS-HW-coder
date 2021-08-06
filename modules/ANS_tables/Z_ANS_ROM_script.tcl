
# For arg parsing to work Vitis HLS needs to be called as: vitis_hls this_script.tcl [args]
set arglen [llength $argv]
set arg_idx 0

#set default parameters
# mode param | 0: all steps | 1: just syn & export
set mode 1


#mode
if { $arg_idx < $arglen } {
  set mode [lindex $argv $arg_idx]
  incr arg_idx
}
puts "Mode : $mode"

set module Z_ANS_ROM

open_project "${module}.hls_prj"
set_top ${module}
add_files src/ANS_tables.cpp -cflags "-DZ_ANS_ROM_TOP"
open_solution "solution1" -flow_target vivado
set_part {xc7z020-clg484-1}
create_clock -period 10 -name default
config_compile -enable_auto_rewind=false
config_export -format ip_catalog -rtl verilog
config_rtl -reset_level low
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
