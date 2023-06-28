############################################################
## This file is generated automatically by Vitis HLS.
## Please DO NOT edit it.
## Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
############################################################

# Create a project
open_project -reset test

# Add design files
add_files kernel.cpp

# Add test bench
add_files -tb tb.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb dataset.dat

# Set the top-level function
set_top test

# Create a solution
open_solution -reset solution -flow_target vitis

# Define technology and clock rate
set_part {xcu50-fsvh2104-2-e}
create_clock -period 3.33 -name default

# Source x_hls.tcl to determine which steps to execute
source directives.tcl

config_interface -m_axi_alignment_byte_size 64 -m_axi_latency 64 -m_axi_max_widen_bitwidth 512
config_rtl -register_reset_num 3
config_export -format ip_catalog -rtl verilog -vivado_clock 3

csim_design -clean
csynth_design
cosim_design -enable_dataflow_profiling
# export_design -flow syn -rtl verilog -format ip_catalog

exit