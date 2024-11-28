open_project -reset kernel

set WF_DIR "/home/blackuntu/projects/FSPX"
set WF_INCLUDE "$WF_DIR/include"
set COMMON "$WF_DIR/tests/Windows/common"

set CFLAGS "-I $WF_INCLUDE -I $COMMON -Wall -Wno-comment -Wno-unknown-pragmas -Wno-unused-label"

add_files kernel.cpp -cflags $CFLAGS -csimflags $CFLAGS
add_files -tb tb.cpp -cflags $CFLAGS -csimflags $CFLAGS

set_top kernel

open_solution -reset solution -flow_target vitis

set_part {xcu50-fsvh2104-2-e}
create_clock -period 3.3333
set_directive_top -name kernel "kernel"

config_interface -m_axi_alignment_byte_size 64 -m_axi_latency 64 -m_axi_max_widen_bitwidth 512
# config_dataflow -override_user_fifo_depth 1024

csim_design -clean
csynth_design
cosim_design -enable_dataflow_profiling

exit
