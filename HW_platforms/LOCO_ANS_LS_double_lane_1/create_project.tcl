#*****************************************************************************************
# Vivado (TM) v2021.2 (64-bit)
#
# create_project.tcl: Tcl script for re-creating project 'hw_platform_project'
#
# Generated by Vivado on Tue Jul 06 10:05:28 CEST 2021
# IP Build 3064653 on Wed Nov 18 14:17:31 MST 2021
#
# This file contains the Vivado Tcl commands for re-creating the project to the state*
# when this script was generated. In order to re-create the project, please source this
# file in the Vivado Tcl Shell.
#
# * Note that the runs in the created project will be configured the same way as the
#   original project, however they will not be launched automatically. To regenerate the
#   run results please launch the synthesis/implementation runs as needed.
#
#*****************************************************************************************
# NOTE: In order to use this script for source control purposes, please make sure that the
#       following files are added to the source control system:-
#
# 1. This project restoration tcl script (create_project.tcl) that was generated.
#
# 2. The following source(s) files that were local or imported into the original project.
#    (Please see the '$orig_proj_dir' and '$origin_dir' variable setting below at the start of the script)
#
#    <none>
#
# 3. The following remote source files that were added to the original project:-
#
#    <none>
#
#*****************************************************************************************

# Check file required for this script exists
proc checkRequiredFiles { origin_dir} {
  set status true
  return $status
}
# Set the reference directory for source file relative paths (by default the value is script directory path)
set origin_dir "."
set ip_repo "../../../"
set target_board pynq_z2

# Use origin directory path location variable, if specified in the tcl shell
if { [info exists ::origin_dir_loc] } {
  set origin_dir $::origin_dir_loc
}

# Set the project name
set _xil_proj_name_ "hw_platform_project"

# Use project name variable, if specified in the tcl shell
if { [info exists ::user_project_name] } {
  set _xil_proj_name_ $::user_project_name
}

variable script_file
set script_file "create_project.tcl"




# Help information for this script
proc print_help {} {
  variable script_file
  puts "\nDescription:"
  puts "Recreate a Vivado project from this script. The created project will be"
  puts "functionally equivalent to the original project for which this script was"
  puts "generated. The script contains commands for creating a project, filesets,"
  puts "runs, adding/importing sources and setting properties on various objects.\n"
  puts "Syntax:"
  puts "$script_file"
  puts "$script_file -tclargs \[--origin_dir <path>\]"
  puts "$script_file -tclargs \[--project_name <name>\]"
  puts "$script_file -tclargs \[--target_board <name>\]"
  puts "$script_file -tclargs \[--help\]\n"
  puts "Usage:"
  puts "Name                   Description"
  puts "-------------------------------------------------------------------------"
  puts "\[--origin_dir <path>\]  Determine source file paths wrt this path. Default"
  puts "                       origin_dir path value is \".\", otherwise, the value"
  puts "                       that was set with the \"-paths_relative_to\" switch"
  puts "                       when this script was generated.\n"
  puts "\[--project_name <name>\] Create project with the specified name. Default"
  puts "                       name is the name of the project from where this"
  puts "                       script was generated.\n"
  puts "\[--target_board\]       Name of target board. To support a new board add:\n"
  puts "                         {board_name}_platform.tcl file defining:\n"
  puts "                         Procedure: \n"
  puts "                              create_root_design "" $block_design_name \n"
  puts "                         Variables: \n"
  puts "                              board \n"
  puts "                              target_part \n"
  puts "                              board_id \n"
  puts "\[--help\]               Print help information for this script"
  puts "-------------------------------------------------------------------------\n"
  exit 0
}

if { $::argc > 0 } {
  for {set i 0} {$i < $::argc} {incr i} {
    set option [string trim [lindex $::argv $i]]
    switch -regexp -- $option {
      "--origin_dir"   { incr i; set origin_dir [lindex $::argv $i] }
      "--project_name" { incr i; set _xil_proj_name_ [lindex $::argv $i] }
      "--target_board" { incr i; set target_board [lindex $::argv $i] }
      "--help"         { print_help }
      default {
        if { [regexp {^-} $option] } {
          puts "ERROR: Unknown option '$option' specified, please type '$script_file -tclargs --help' for usage info.\n"
          return 1
        }
      }
    }
  }
}

puts " script ARGS:"
puts "origin_dir: ${origin_dir}"
puts "project_name: ${_xil_proj_name_}"
puts "target_board: ${target_board}"


proc procExists p {
   return uplevel 1 [expr {[llength [info procs $p]] > 0}]
}


set bd_source_tcl "../bd_scripts/${target_board}_platform.tcl"



if  { ! [file exists $bd_source_tcl] } {
  puts " Error: $target_board is not a supported board "

  puts "   File $bd_source_tcl does not exist"
  quit
}



############### Create project ###############

# Set the directory path for the original project from where this script was exported
set orig_proj_dir "[file normalize "$origin_dir/hw_platform_project"]"

# Check for paths and files needed for project creation
set validate_required 0
if { $validate_required } {
  if { [checkRequiredFiles $origin_dir] } {
    puts "Tcl file $script_file is valid. All files required for project creation is accessible. "
  } else {
    puts "Tcl file $script_file is not valid. Not all files required for project creation is accessible. "
    return
  }
}

# Create project
create_project ${_xil_proj_name_} .

# Set the directory path for the new project
set proj_dir [get_property directory [current_project]]


#### ADD IP Repo
set_property  ip_repo_paths $ip_repo [current_project]
update_ip_catalog -rebuild


# source target:
# create_root_design "" $block_design_name
# Variables:
#      board
#      target_part
#      board_id

source $bd_source_tcl

#check proc and variables are defined
if {![info exists board]} {
  puts "Error: Variable \"board\" has to be defined in $bd_source_tcl"
  quit
}

if {![info exists target_part]} {
  puts "Error: Variable \"target_part\" has to be defined in $bd_source_tcl"
  quit
}

if {![info exists board_id]} {
  puts "Error: Variable \"board_id\" has to be defined in $bd_source_tcl"
  quit
}


if {![procExists create_root_design]} {
  puts "Error: Procedure \"create_root_design\" has to be defined in $bd_source_tcl"
  quit
}


# Set project properties
set obj [current_project]
set_property -name "part" -value "$target_part" -objects $obj
set_property -name "board_part" -value "$board" -objects $obj
set_property -name "default_lib" -value "xil_defaultlib" -objects $obj
set_property -name "enable_vhdl_2008" -value "1" -objects $obj
set_property -name "ip_cache_permissions" -value "read write" -objects $obj
set_property -name "ip_output_repo" -value "$proj_dir/${_xil_proj_name_}.cache/ip" -objects $obj
set_property -name "mem.enable_memory_map_generation" -value "1" -objects $obj
set_property -name "platform.board_id" -value $board_id -objects $obj
set_property -name "sim.central_dir" -value "$proj_dir/${_xil_proj_name_}.ip_user_files" -objects $obj
set_property -name "sim.ip.auto_export_scripts" -value "1" -objects $obj
set_property -name "simulator_language" -value "Mixed" -objects $obj
# VHDL synthesis fails for i/odma optimized for read-only/write-only
set_property -name "target_language" -value "Verilog" -objects $obj

# Create 'sources_1' fileset (if not found)
if {[string equal [get_filesets -quiet sources_1] ""]} {
  create_fileset -srcset sources_1
}

# Set 'sources_1' fileset object
set obj [get_filesets sources_1]
# Empty (no sources present)

# Set 'sources_1' fileset properties
set obj [get_filesets sources_1]

# Create 'constrs_1' fileset (if not found)
if {[string equal [get_filesets -quiet constrs_1] ""]} {
  create_fileset -constrset constrs_1
}

# Set 'constrs_1' fileset object
set obj [get_filesets constrs_1]

# Empty (no sources present)

# Set 'constrs_1' fileset properties
set obj [get_filesets constrs_1]

# Create 'sim_1' fileset (if not found)
if {[string equal [get_filesets -quiet sim_1] ""]} {
  create_fileset -simset sim_1
}

# Set 'sim_1' fileset object
set obj [get_filesets sim_1]
# Empty (no sources present)

# Set 'sim_1' fileset properties
set obj [get_filesets sim_1]
set_property -name "hbs.configure_design_for_hier_access" -value "1" -objects $obj

# Set 'utils_1' fileset object
set obj [get_filesets utils_1]
# Empty (no sources present)

# Set 'utils_1' fileset properties
set obj [get_filesets utils_1]





######## Run configs ###############

# Create 'synth_1' run (if not found)
if {[string equal [get_runs -quiet synth_1] ""]} {
      create_run -name synth_1 -part xc7z020clg400-1 -flow {Vivado Synthesis 2021} -strategy "Flow_PerfOptimized_high" -report_strategy {No Reports} -constrset constrs_1
} else {
  set_property strategy "Flow_PerfOptimized_high" [get_runs synth_1]
  set_property flow "Vivado Synthesis 2021" [get_runs synth_1]
}

set obj [get_runs synth_1]
set_property set_report_strategy_name 1 $obj
set_property report_strategy {Vivado Synthesis Default Reports} $obj
set_property set_report_strategy_name 0 $obj

set obj [get_runs synth_1]
# set the current synth run
current_run -synthesis [get_runs synth_1]
# set_property -name "strategy" -value "Vivado Synthesis Defaults" -objects $obj

# Create 'impl_1' run (if not found)
if {[string equal [get_runs -quiet impl_1] ""]} {
    create_run -name impl_1 -part xc7z020clg400-1 -flow {Vivado Implementation 2021} -strategy "Performance_ExploreWithRemap" -report_strategy {No Reports} -constrset constrs_1 -parent_run synth_1
} else {
  set_property strategy "Performance_ExploreWithRemap" [get_runs impl_1]
  set_property flow "Vivado Implementation 2021" [get_runs impl_1]
}

set obj [get_runs impl_1]
set_property set_report_strategy_name 1 $obj
set_property report_strategy {Vivado Implementation Default Reports} $obj
set_property set_report_strategy_name 0 $obj

set obj [get_runs impl_1]

# set the current impl run
current_run -implementation [get_runs impl_1]




#set build strategy
set obj [get_runs synth_1]
set_property -name "strategy" -value "Flow_PerfOptimized_high" -objects $obj
set_property -name "steps.synth_design.args.directive" -value "PerformanceOptimized" -objects $obj
set_property -name "steps.synth_design.args.fsm_extraction" -value "one_hot" -objects $obj
set_property -name "steps.synth_design.args.keep_equivalent_registers" -value "1" -objects $obj
set_property -name "steps.synth_design.args.resource_sharing" -value "off" -objects $obj
set_property -name "steps.synth_design.args.no_lc" -value "1" -objects $obj
set_property -name "steps.synth_design.args.shreg_min_size" -value "5" -objects $obj

set obj [get_runs impl_1]
set_property -name "strategy" -value "Performance_ExploreWithRemap" -objects $obj
set_property -name "steps.opt_design.args.directive" -value "ExploreWithRemap" -objects $obj
set_property -name "steps.place_design.args.directive" -value "Explore" -objects $obj
set_property -name "steps.phys_opt_design.args.directive" -value "AggressiveExplore" -objects $obj
set_property -name "steps.route_design.args.directive" -value "NoTimingRelaxation" -objects $obj
set_property -name "steps.route_design.args.more options" -value "-tns_cleanup" -objects $obj
set_property -name "steps.post_route_phys_opt_design.is_enabled" -value "1" -objects $obj
set_property -name "steps.post_route_phys_opt_design.args.directive" -value "AggressiveExplore" -objects $obj

set_property -name "steps.write_bitstream.args.readback_file" -value "0" -objects [get_runs impl_1]
set_property -name "steps.write_bitstream.args.verbose" -value "0" -objects [get_runs impl_1]




######## Build###############

######## create bd
set block_design_name platform

create_root_design ""

if { [llength [ get_files  "${block_design_name}.bd" ]]  == 0 } {
  puts " Error: Block design should be named: ${block_design_name}.bd "
  puts "        No platform.bd file was found, quitting..."
  quit
}

save_bd_design
assign_bd_address
validate_bd_design

set_property SYNTH_CHECKPOINT_MODE "Hierarchical" [ get_files "${block_design_name}.bd" ]
make_wrapper -files [get_files "${block_design_name}.bd"] -import -fileset sources_1 -top

# generate_target all [get_files  platform.bd ]

launch_runs synth_1 -jobs 5
wait_on_run [get_runs synth_1]

#launch_runs  impl_1 -jobs 5
launch_runs -to_step write_bitstream impl_1 -jobs 5
wait_on_run [get_runs impl_1]

#open_run impl_1
#phys_opt_design -retime -placement_opt -routing_opt -critical_cell_opt -insert_negative_edge_ffs
#launch_runs -to_step write_bitstream impl_1 -jobs 5
#wait_on_run [get_runs impl_1]

#generate xsa (includes .hwh for pynq)
write_hw_platform -fixed -include_bit -force -file "${block_design_name}.xsa"

#generate reports
open_run impl_1
report_utilization -hierarchical -hierarchical_depth 4 -file "util_report.txt"
report_timing_summary -nworst 10 -file "timing_summary.txt"
close_project
quit
