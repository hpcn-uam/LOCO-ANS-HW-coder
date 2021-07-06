open_project subsymbol_gen.hls_prj
set_top sub_symbol_gen
add_files src/subsym_gen.cpp -cflags "-DSUB_SYMBOL_GEN_TOP"
add_files -tb src/test.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../input_buffers/src/input_buffers.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
open_solution "solution1" -flow_target vivado
set_part {xc7z020-clg484-1}
create_clock -period 10 -name default
# source "./directives.tcl"
csim_design -clean
csynth_design
cosim_design -enable_dataflow_profiling -trace_level all
export_design -format ip_catalog
