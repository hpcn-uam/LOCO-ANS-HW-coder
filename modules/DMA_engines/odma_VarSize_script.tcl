# ==================================================================================================
# -----------------------------------------------------------------------------
# Copyright (c) 2018. All rights reserved
# -----------------------------------------------------------------------------
#  @Filename      : odma_VarSize_script.tcl
#  @Author        : Tobías Alonso
#  @Email         : tobi.alonso@gmail.com
#  @Created On    : 2021-07-02 11:10:04
#  @Description   :
# 
# 
# 
#  Last Modified : 2021-07-02 11:12:47 
# 
#  Revision      : 
# 
#  
# ==================================================================================================

open_project odma_VarSize.hls_prj
set_top odma_VarSize
add_files src/dma.cpp
add_files -tb src/test_odma_VarSize.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
open_solution "solution1" -flow_target vivado
set_part {xc7z020clg400-1}
create_clock -period 10 -name default
#source "./odma_VarSize.hls_prj/solution1/directives.tcl"
csim_design -clean
csynth_design
cosim_design -enable_dataflow_profiling -trace_level all
export_design -format ip_catalog
