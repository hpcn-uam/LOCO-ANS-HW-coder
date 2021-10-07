
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
    set period  20
    set clk_uncertainty  2
  }
  1 {
    set sol_name "zcu104"
    set part xczu7ev-ffvc1156-2-e
    set period 10
    set clk_uncertainty  1
  }
  default {
    puts "Error: target id not supported"
    quit
  }
}

set ip_version $target_id

puts "Target : $sol_name"
puts "Period : $period"
puts "Clk uncertainty : $clk_uncertainty"
puts "Part : $part"
puts "Mode : $mode"

set module LOCO_decorrelator


open_project "${module}.hls_prj"
set_top "${module}"
add_files src/${module}.cpp -cflags "-DLOCO_DECORRELATOR_TOP -Wmissing-field-initializers "
add_files -tb src/${module}_test.cpp -cflags "-Wno-unknown-pragmas -Wmissing-field-initializers " -csimflags "-std=c++14 -fexceptions -Wno-unknown-pragmas -Wmissing-field-initializers "
add_files -tb src/sw_implementation.cpp -cflags "-Wno-unknown-pragmas -Wmissing-field-initializers " -csimflags "-Wno-unknown-pragmas -Wmissing-field-initializers "
open_solution "${sol_name}" -flow_target vivado
set_part "$part"
create_clock -period ${period} -name default
set_clock_uncertainty ${clk_uncertainty}
config_compile -enable_auto_rewind=false
config_schedule  -verbose
config_export -format ip_catalog -rtl verilog -library loco_ans -vendor HPCN -version "1.${ip_version}"

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
