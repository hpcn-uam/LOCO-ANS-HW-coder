open_project output_data_interface.hls_prj
set_top output_data_interface
add_files src/TSG_coder.cpp -cflags "-DOUTPUT_DATA_INTERFACE_TOP"
add_files ../input_buffers/src/input_buffers.cpp
add_files -tb src/test.cpp
open_solution "solution1" -flow_target vivado
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
config_compile -enable_auto_rewind=false
#source "./src/directives.tcl"
csynth_design
export_design -format ip_catalog
