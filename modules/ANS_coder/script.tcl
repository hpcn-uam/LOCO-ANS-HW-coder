open_project ANS_coder.hls_prj
set_top ANS_coder_top
add_files src/ANS_coder.cpp -cflags "-DANS_CODER_TOP"
add_files -tb src/test.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../subsym_gen/src/subsym_gen.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../input_buffers/src/input_buffers.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
open_solution "solution1" -flow_target vivado
set_part {xc7z020-clg484-1}
create_clock -period 10 -name default
config_export -format ip_catalog -rtl verilog
# source "./directives.tcl"
csim_design -clean
csynth_design
cosim_design -enable_dataflow_profiling -trace_level all
export_design -rtl verilog -format ip_catalog
# export_design -flow impl -rtl verilog -format ip_catalog
