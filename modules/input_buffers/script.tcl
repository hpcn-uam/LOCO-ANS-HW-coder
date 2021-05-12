open_project input_buffers.hls_prj
set_top input_buffers
add_files src/input_buffers.cpp -cflags "-DINPUT_BUFFERS_TOP"
add_files -tb src/test.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
open_solution "solution1" -flow_target vivado
set_part {xc7z020-clg484-1}
create_clock -period 10 -name default
# source "./directives.tcl"
csim_design -clean
csynth_design
cosim_design -enable_dataflow_profiling -trace_level all
export_design -flow syn -format ip_catalog