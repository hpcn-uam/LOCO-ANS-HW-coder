open_project pack_out_bytes.hls_prj
set_top pack_out_bytes_top
add_files src/pack_out_bytes.cpp -cflags "-DPACK_OUT_BYTES_TOP"
add_files -tb src/test.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
open_solution "solution1" -flow_target vivado
set_part {xc7z020-clg484-1}
create_clock -period 10 -name default
# source "./directives.tcl"
csim_design -clean
csynth_design
cosim_design -enable_dataflow_profiling -trace_level all
export_design -flow syn -format ip_catalog