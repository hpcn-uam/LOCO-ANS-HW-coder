
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

set module LOCO_decorrelator_LS


open_project "${module}.hls_prj"
set_top "${module}"
add_files src/LOCO_decorrelator.cpp -cflags "-DLOCO_DECORRELATOR_LS_TOP -Wmissing-field-initializers "
add_files -tb src/LOCO_decorrelator_test.cpp -cflags "-DLOCO_DECORRELATOR_LS_TOP -Wno-unknown-pragmas -Wmissing-field-initializers " -csimflags " -DLOCO_DECORRELATOR_LS_TOP -std=c++14 -fexceptions -Wno-unknown-pragmas -Wmissing-field-initializers "
add_files -tb src/sw_implementation.cpp -cflags "-Wno-unknown-pragmas -Wmissing-field-initializers " -csimflags "-Wno-unknown-pragmas -Wmissing-field-initializers "
open_solution "solution1" -flow_target vivado
set_part {xc7z020-clg484-1}
create_clock -period 17 -name default
set_clock_uncertainty 2
config_compile -enable_auto_rewind=false
config_schedule  -verbose
config_export -format ip_catalog -rtl verilog -library loco_ans -vendor HPCN -version "1.0" -vivado_synth_strategy "Flow_PerfOptimized_high" -vivado_optimization_level 3 -vivado_impl_strategy "Performance_ExtraTimingOpt" -vivado_phys_opt all  -vivado_report_level 2

# source "./directives.tcl"
if { $mode == 0 } {
  csim_design -clean
}

if { $mode == 1 } {
  csynth_design
}

if { $mode == 2 } {
  cosim_design -enable_dataflow_profiling -trace_level all
}

if { $mode == 3 } {
  export_design -format ip_catalog
}
