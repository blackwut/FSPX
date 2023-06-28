FSPX = ~/FSPX_blackwut/include

# Options for TARGET: sw_emu, hw_emu and hw
TARGET ?= hw_emu
DEVICE ?= u50
PLATFORM ?= xilinx_$(DEVICE)_gen3x16_xdma_5_202210_1

PLATFORM_REPO_PATHS ?= /opt/xilinx/platforms
PFM := $(PLATFORM_REPO_PATHS)/$(PLATFORM)/$(PLATFORM).xpfm

VPP = v++
VPPINCLUDES := -I. -I$(FSPX)
VPPFLAGS := --platform $(PFM) -t $(TARGET) -s -g
VPPCONNECT :=

# Needed to connect kernels
VPP_LINK_FLAGS :=

# COMPUTE
K_COMPUTE_NAME := compute
K_COMPUTE_FILE := $(K_COMPUTE_NAME).cpp
K_COMPUTE_XO := $(K_COMPUTE_NAME).xo
K_COMPUTE_XCLBIN := $(K_COMPUTE_NAME).xclbin

# HOST
HOSTCXX = g++
HOSTCXXFLAGS = -std=c++14 -I$(XILINX_VIVADO)/include -I$(XILINX_XRT)/include/ -g -O0 -Wall -fmessage-length=0
HOSTLDFLAGS = -L /usr/lib/x86_64-linux-gnu/ -L$(XILINX_XRT)/lib/ -L$(XILINX_XRT)/lib/lnx64.o -lOpenCL -pthread -lrt -lstdc++
HOSTINCLUDES = -I. -I /tools/Xilinx/Vitis_HLS/2022.1/include

host: main.cpp
	$(HOSTCXX) $(HOSTCXXFLAGS) $(HOSTINCLUDES) -o main main.cpp $(HOSTLDFLAGS)

$(K_COMPUTE_XO): $(K_COMPUTE_FILE)
	$(VPP) --kernel $(K_COMPUTE_NAME) $(VPPFLAGS) $(VPPCONNECT) $(VPPINCLUDES) -c -o $@ $<

$(K_COMPUTE_XCLBIN): $(K_COMPUTE_XO)
	$(VPP) -l $(VPPFLAGS) $(VPPCONNECT) $(VPP_LINK_FLAGS) -o $(K_COMPUTE_XCLBIN) $(+)


emconfig:emconfig.json
emconfig.json:
	emconfigutil --platform $(PLATFORM)

all: emconfig $(KERNEL_XCLBIN) host
run: all
	XCL_EMULATION_MODE=$(TARGET) ./main

run_emu: all
	XCL_EMULATION_MODE=hw_emu ./main sd.xclbin 1 1 1 1024 1024

clean:
	$(RM) -rf _x .run .Xil *.ltx *.log *.info *compile_summary* vitis_analyzer* *link_summary* *.o main

cleanall:
	$(RM) -rf sd_host *.o *.xo _x .run .Xil *.xclbin *.ltx *.log *.info *compile_summary* vitis_analyzer* *link_summary* *.o main emconfig.json device_trace_0.csv *.xclbin.package_summary opencl_trace.csv xrt.run_summary summary.csv .ipcache waveform_prefs.json
