FSPX = ~/FSPX/include

# Options for TARGET: sw_emu, hw_emu and hw
TARGET ?= sw_emu
DEVICE ?= u50
PLATFORM ?= xilinx_$(DEVICE)_gen3x16_xdma_5_202210_1

PLATFORM_REPO_PATHS ?= /opt/xilinx/platforms
PFM := $(PLATFORM_REPO_PATHS)/$(PLATFORM)/$(PLATFORM).xpfm

VPP = v++
VPPINCLUDES := -I. -I../common -I$(FSPX)
VPPFLAGS := --platform $(PFM) -t $(TARGET) -s -g
VPPCONNECT := --config memory_connectivity.ini

# Needed to connect kernels
VPP_LINK_FLAGS := --config axis_connectivity.cfg

# Memory READER
K_MEMORY_READER_NAME := memory_reader
K_MEMORY_READER_FILE := $(K_MEMORY_READER_NAME).cpp
K_MEMORY_READER_XO := $(K_MEMORY_READER_NAME).xo

# COMPUTE
K_COMPUTE_NAME := compute
K_COMPUTE_FILE := $(K_COMPUTE_NAME).cpp
K_COMPUTE_XO := $(K_COMPUTE_NAME).xo

# Memory WRITER
K_MEMORY_WRITER_NAME := memory_writer
K_MEMORY_WRITER_FILE := $(K_MEMORY_WRITER_NAME).cpp
K_MEMORY_WRITER_XO := $(K_MEMORY_WRITER_NAME).xo

KERNEL_NAME := sd
KERNEL_LINK_XCLBIN := $(KERNEL_NAME).link.xclbin
KERNEL_XO := $(KERNEL_NAME).xo
KERNEL_XCLBIN := $(KERNEL_NAME).xclbin

# HOST
HOSTCXX = g++
HOSTCXXFLAGS = -std=c++14 -I$(XILINX_VIVADO)/include -I$(XILINX_XRT)/include/ -g
HOSTLDFLAGS = -L /usr/lib/x86_64-linux-gnu/ -L$(XILINX_XRT)/lib/ -L$(XILINX_XRT)/lib/lnx64.o -lOpenCL -pthread -lrt -lstdc++
HOSTINCLUDES = -I. -I ~/Vitis_Libraries/utils/ext/xcl2 -I /tools/Xilinx/Vitis_HLS/2022.1/include

XCL2CPP = ~/Vitis_Libraries/utils/ext/xcl2/xcl2.cpp

xcl2.o: $(XCL2CPP)
	$(HOSTCXX) $(HOSTCXXFLAGS) $(HOSTINCLUDES) $< -c $(HOSTLDFLAGS)

host: xcl2.o main.cpp
	$(HOSTCXX) $(HOSTCXXFLAGS) $(HOSTINCLUDES) xcl2.o -o main main.cpp $(HOSTLDFLAGS)


$(K_MEMORY_READER_XO): $(K_MEMORY_READER_FILE)
	$(VPP) --kernel $(K_MEMORY_READER_NAME) $(VPPFLAGS) $(VPPINCLUDES) -c -o $@ $<

$(K_COMPUTE_XO): $(K_COMPUTE_FILE)
	$(VPP) --kernel $(K_COMPUTE_NAME) $(VPPFLAGS) $(VPPCONNECT) $(VPPINCLUDES) -c -o $@ $<

$(K_MEMORY_WRITER_XO): $(K_MEMORY_WRITER_FILE)
	$(VPP) --kernel $(K_MEMORY_WRITER_NAME) $(VPPFLAGS) $(VPPCONNECT) $(VPPINCLUDES) -c -o $@ $<

$(KERNEL_XCLBIN): $(K_MEMORY_READER_XO) $(K_COMPUTE_XO) $(K_MEMORY_WRITER_XO)
	$(VPP) -l $(VPPFLAGS) $(VPPCONNECT) $(VPP_LINK_FLAGS) -o $(KERNEL_LINK_XCLBIN) $(+)
	$(VPP) -p $(KERNEL_LINK_XCLBIN) $(VPPFLAGS) $(VPPCONNECT) -o $(KERNEL_XCLBIN)


emconfig:emconfig.json
emconfig.json:
	emconfigutil --platform $(PLATFORM)

all: emconfig $(KERNEL_XCLBIN) host
run: all
	XCL_EMULATION_MODE=$(TARGET) ./main

clean:
	$(RM) -rf _x .run .Xil *.ltx *.log *.info *compile_summary* vitis_analyzer* *link_summary* *.o main

cleanall:
	$(RM) -rf sd_host *.o *.xo _x .run .Xil *.xclbin *.ltx *.log *.info *compile_summary* vitis_analyzer* *link_summary* *.o main emconfig.json device_trace_0.csv *.xclbin.package_summary opencl_trace.csv xrt.run_summary summary.csv .ipcache waveform_prefs.json
