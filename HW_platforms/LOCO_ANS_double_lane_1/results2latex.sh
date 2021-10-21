#!/bin/bash
DEF_COLOR="\e[39m"
MSJ_COLOR="\e[32m"
WAR_COLOR="\e[33m"
ERR_COLOR="\e[31m"
Print_Msg(){
  time=$(date|awk '{print $4}')
  echo -e " ${time}: ${MSJ_COLOR} $@ ${DEF_COLOR}"
}

Print_War(){
  time=$(date|awk '{print $4}')
  echo -e " ${time}: ${WAR_COLOR} $@ ${DEF_COLOR}"
}

Print_Err(){
  time=$(date|awk '{print $4}')
  echo -e " ${time}: ${ERR_COLOR} $@ ${DEF_COLOR}"
}

MODULES=( LOCO_decorrelator_0 LOCO_decorrelator_1 St_idx_compute_0 St_idx_compute_1 TSG_coder_double_lane_0 first_px_fifo first_px_fifo1 symbol_fifo symbol_fifo1 )

target="pynq_z2"

if ! [[ -z $1  ]]; then
  target="$1"
fi

vivado_folder="vivado_$target"

config="LOCO-ANS6"

lanes="2"

#round to 1 decimal
if [[ $target == "zcu104" ]]; then
  clk_0_target_period=`cat $vivado_folder/timing_summary.txt|grep "clk_pl_0  {0.000 " |awk -F " " '//{print $4}'`
  clk_1_target_period=`cat $vivado_folder/timing_summary.txt|grep "clk_pl_2  {0.000 " |awk -F " " '//{print $4}'`
  timing_summary=`cat $vivado_folder/timing_summary.txt|grep "| Intra Clock Table" -A9`
  clk_0_wns=`echo "$timing_summary" | awk -F " " '/clk_pl_0/{print $2}'`
  clk_1_wns=`echo "$timing_summary" | awk -F " " '/clk_pl_2/{print $2}'`
else
  clk_0_target_period=`cat $vivado_folder/timing_summary.txt|grep "clk_fpga_0  {0.000 " |awk -F " " '//{print $4}'`
  clk_1_target_period=`cat $vivado_folder/timing_summary.txt|grep "clk_fpga_1  {0.000 " |awk -F " " '//{print $4}'`
  timing_summary=`cat $vivado_folder/timing_summary.txt|grep "| Intra Clock Table" -A9`
  clk_0_wns=`echo "$timing_summary" | awk -F " " '/clk_fpga_0/{print $2}'`
  clk_1_wns=`echo "$timing_summary" | awk -F " " '/clk_fpga_1/{print $2}'`
fi

clk_0=$(echo "scale=1; 1000 /(${clk_0_target_period} - ${clk_0_wns})" |bc )
clk_1=$(echo "scale=1; 1000 /(${clk_1_target_period} - ${clk_1_wns})" |bc )


get_sum_of_util(){ # position of util in text table
  pos=$1
  acc=0
  for module in "${MODULES[@]}"; do
    cmd="/${module} /{print \$${pos}}"
    # echo "  | cmd: $cmd"
    val=`cat $vivado_folder/util_report.txt| awk -F " " "$cmd"`
    # echo "   | acc : $acc| val : $val"
    acc=`echo "scale=1; ${acc} + ${val}" |bc`
  done
  echo "$acc"
}

# get_sum_of_util 6

LUT=`get_sum_of_util 6`
FF=`get_sum_of_util 14`

BRAM36=`get_sum_of_util 16`
BRAM18=`get_sum_of_util 18`
BRAM=`echo "scale=1; ${BRAM36} + ${BRAM18}/2" |bc`

if [[ $target == "zcu104" ]]; then
  DSP=`get_sum_of_util 22`
else
  DSP=`get_sum_of_util 20`
fi

echo  "      Part & Coder config & Lanes &  Clk0/1 (MHz) &   LUT  &    FF  &  BRAM & DSP "
printf "%10s & %12s & %5d & %5s / %5s & %6d & %6d & %5s & %3d \\\\\\ \n" $target  $config  $lanes  $clk_0  $clk_1   $LUT   $FF  $BRAM  $DSP 


  