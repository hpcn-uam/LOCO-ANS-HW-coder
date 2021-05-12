open_project TSG_coder.hls_prj
set_top TSG_coder
add_files src/TSG_coder.cpp
add_files module/input_buffers/src/input_buffers.cpp
add_files module/subsym_gen/src/subsym_gen.cpp
add_files module/ANS_coder/src/ANS_coder.cpp
add_files -tb src/test.cpp
open_solution "solution1" -flow_target vivado
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
#source "./src/directives.tcl"
csim_design -clean
csynth_design
cosim_design
export_design -flow syn -format ip_catalog
