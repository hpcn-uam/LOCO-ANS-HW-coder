#
# Common BITGEN related settings for TE0726
#

set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]
set_property CFGBVS VCCO [current_design]
set_property BITSTREAM.CONFIG.UNUSEDPIN PULLUP [current_design]


## CSI

#set_property -dict {PACKAGE_PIN N11 IOSTANDARD LVDS_25} [get_ports csi_c_clk_p]

#set_property PACKAGE_PIN M9 [get_ports {csi_d_lp_n[0]}]
#set_property IOSTANDARD HSUL_12 [get_ports {csi_d_lp_n[0]}]
#set_property PULLDOWN true [get_ports {csi_d_lp_n[0]}]

#set_property PACKAGE_PIN N9 [get_ports {csi_d_lp_p[0]}]
#set_property IOSTANDARD HSUL_12 [get_ports {csi_d_lp_p[0]}]
#set_property PULLDOWN true [get_ports {csi_d_lp_p[0]}]

#set_property -dict {PACKAGE_PIN M10 IOSTANDARD LVDS_25} [get_ports {csi_d_p[0]}]

#set_property -dict {PACKAGE_PIN P13 IOSTANDARD LVDS_25} [get_ports {csi_d_p[1]}]

#set_property INTERNAL_VREF 0.6 [get_iobanks 34]
#create_clock -period 6.250 -name csi_clk -add [get_ports csi_c_clk_p]



## Timings

#set_clock_groups -asynchronous -group [get_clocks clk_fpga_3] -group [get_clocks clk_fpga_0]
#set_clock_groups -asynchronous -group [get_clocks clk_fpga_0] -group [get_clocks clk_fpga_3]


#GPIO

set_property IOSTANDARD LVCMOS33 [get_ports {GPIO_0_tri_io[*]}]
# GPIO Pins
# GPIO2
set_property PACKAGE_PIN K15 [get_ports {GPIO_0_tri_io[0]}]
# GPIO3
set_property PACKAGE_PIN J14 [get_ports {GPIO_0_tri_io[1]}]
# GPIO4
set_property PACKAGE_PIN H12 [get_ports {GPIO_0_tri_io[2]}]
# GPIO5
set_property PACKAGE_PIN N14 [get_ports {GPIO_0_tri_io[3]}]
# GPIO6
set_property PACKAGE_PIN R15 [get_ports {GPIO_0_tri_io[4]}]
# GPIO7
set_property PACKAGE_PIN L14 [get_ports {GPIO_0_tri_io[5]}]
# GPIO8
set_property PACKAGE_PIN L15 [get_ports {GPIO_0_tri_io[6]}]
# GPIO9
set_property PACKAGE_PIN J13 [get_ports {GPIO_0_tri_io[7]}]
# GPIO10
set_property PACKAGE_PIN H14 [get_ports {GPIO_0_tri_io[8]}]
# GPIO11
set_property PACKAGE_PIN J15 [get_ports {GPIO_0_tri_io[9]}]
# GPIO12
set_property PACKAGE_PIN M15 [get_ports {GPIO_0_tri_io[10]}]
# GPIO13
set_property PACKAGE_PIN R13 [get_ports {GPIO_0_tri_io[11]}]
# GPIO16
set_property PACKAGE_PIN L13 [get_ports {GPIO_0_tri_io[12]}]
# GPIO17
set_property PACKAGE_PIN G11 [get_ports {GPIO_0_tri_io[13]}]
# GPIO18
set_property PACKAGE_PIN H11 [get_ports {GPIO_0_tri_io[14]}]
# GPIO19
set_property PACKAGE_PIN R12 [get_ports {GPIO_0_tri_io[15]}]
# GPIO20
set_property PACKAGE_PIN M14 [get_ports {GPIO_0_tri_io[16]}]
# GPIO21
set_property PACKAGE_PIN P15 [get_ports {GPIO_0_tri_io[17]}]
# GPIO22
set_property PACKAGE_PIN H13 [get_ports {GPIO_0_tri_io[18]}]
# GPIO23
set_property PACKAGE_PIN J11 [get_ports {GPIO_0_tri_io[19]}]
# GPIO24
set_property PACKAGE_PIN K11 [get_ports {GPIO_0_tri_io[20]}]
# GPIO25
set_property PACKAGE_PIN K13 [get_ports {GPIO_0_tri_io[21]}]
# GPIO26
set_property PACKAGE_PIN L12 [get_ports {GPIO_0_tri_io[22]}]
# GPIO27
set_property PACKAGE_PIN G12 [get_ports {GPIO_0_tri_io[23]}]

# False path
#set_false_path -from [get_pins video_coder_i/video_in/camera_rx/csi_to_axis_0/U0/lane_align_inst/err_req_reg/C] -to [get_pins video_coder_i/video_in/camera_rx/csi2_d_phy_rx_0/U0/clock_upd_req_reg/D]

# Debug

#create_pblock pblock_LHE_quantizer_0
#add_cells_to_pblock [get_pblocks pblock_LHE_quantizer_0] [get_cells -quiet [list system_block_i/video_in/LHE_Codec/LHE_quantizer_0]]
#resize_pblock [get_pblocks pblock_LHE_quantizer_0] -add {SLICE_X22Y75:SLICE_X43Y99}
#resize_pblock [get_pblocks pblock_LHE_quantizer_0] -add {DSP48_X1Y30:DSP48_X1Y39}
#resize_pblock [get_pblocks pblock_LHE_quantizer_0] -add {RAMB18_X1Y30:RAMB18_X2Y39}
#resize_pblock [get_pblocks pblock_LHE_quantizer_0] -add {RAMB36_X1Y15:RAMB36_X2Y19}
#set_property C_CLK_INPUT_FREQ_HZ 300000000 [get_debug_cores dbg_hub]
#set_property C_ENABLE_CLK_DIVIDER false [get_debug_cores dbg_hub]
#set_property C_USER_SCAN_CHAIN 1 [get_debug_cores dbg_hub]
#connect_debug_port dbg_hub/clk [get_nets clk]


