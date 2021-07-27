open_project TSG_input_adapter.hls_prj
set_top TSG_input_adapter
add_files src/TSG_coder.cpp -cflags "-DTSG_INPUT_ADAPTER_TOP"
add_files ../input_buffers/src/input_buffers.cpp
add_files -tb src/test.cpp
open_solution "solution1" -flow_target vivado
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
config_compile -enable_auto_rewind=false
#source "./src/directives.tcl"
csynth_design
export_design -format ip_catalog
