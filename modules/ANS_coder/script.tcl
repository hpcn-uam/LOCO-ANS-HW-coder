############################################################
## This file is generated automatically by Vitis HLS.
## Please DO NOT edit it.
## Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
############################################################
open_project ANS_coder.hls_prj
set_top ANS_coder
add_files src/ANS_coder.cpp
add_files -tb ../input_buffers/src/input_buffers.cpp
add_files -tb ../subsym_gen/src/subsym_gen.cpp
add_files -tb src/test.cpp
open_solution "solution1" -flow_target vivado
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
#source "./ANS_coder.hls_prj/solution1/directives.tcl"
csim_design -clean
csynth_design
cosim_design
export_design -format ip_catalog
