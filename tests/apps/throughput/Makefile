FSPX_INCLUDE := $(HOME)/FSPX_blackwut/include
VITIS_INCLUDE := $(XILINX_VITIS)/include
VIVADO_INCLUDE := $(XILINX_VIVADO)/include
XRT_INCLUDE := $(XILINX_XRT)/include

XRT_LIB := $(XILINX_XRT)/lib
XRT_LIB_O := $(XRT_LIB)lnx64.o
CXX_LIBS := -lOpenCL -pthread -lrt -lstdc++


# Options for TARGET: sw_emu, hw_emu and hw
TARGET ?= sw_emu
DEVICE ?= u50
PLATFORM ?= xilinx_$(DEVICE)_gen3x16_xdma_5_202210_1
PLATFORM_REPO_PATHS ?= /opt/xilinx/platforms
PFM := $(PLATFORM_REPO_PATHS)/$(PLATFORM)/$(PLATFORM).xpfm

# Output directories
KERNEL_NAME ?= th
KERNELS_DIR := ./kernels
OUT_DIR = $(KERNELS_DIR)/${KERNEL_NAME}/$(TARGET)
TEMP_DIR := $(OUT_DIR)/temp
LOG_DIR := $(OUT_DIR)/log
REPORT_DIR := $(OUT_DIR)/report
BUILD_DIR := $(OUT_DIR)/build

# OUTPUT KERNEL
KERNEL_LINK_XCLBIN := $(BUILD_DIR)/$(KERNEL_NAME).link.xclbin
KERNEL_XO := $(BUILD_DIR)/$(KERNEL_NAME).xo
KERNEL_XCLBIN := $(BUILD_DIR)/$(KERNEL_NAME).xclbin

# Vitis compiler
VPP = v++
VPP_INCLUDES := -I. -I$(FSPX_INCLUDE)
VPP_FLAGS := --platform $(PFM) --target $(TARGET)
VPP_FLAGS += --debug
VPP_FLAGS += --save-temps
VPP_FLAGS += --temp_dir $(TEMP_DIR)
VPP_FLAGS += --log_dir $(LOG_DIR)
VPP_FLAGS += --report_dir $(REPORT_DIR)
VPP_FLAGS += --config connectivity_memory.ini
VPP_LINK_FLAGS := --config connectivity_axis.ini
VPP_PACKAGE_FLAGS := --package.out_dir $(BUILD_DIR)

# MEMORY_READER
MEMORY_READER_CUS ?= 4
MEMORY_READER_KERNEL_NAME := memory_reader
MEMORY_READER_CPP := $(MEMORY_READER_KERNEL_NAME).cpp
MEMORY_READER_XO := $(BUILD_DIR)/$(MEMORY_READER_KERNEL_NAME).xo

# COMPUTE
COMPUTE_KERNEL_NAME := compute
COMPUTE_CPP := $(COMPUTE_KERNEL_NAME).cpp
COMPUTE_XO := $(BUILD_DIR)/$(COMPUTE_KERNEL_NAME).xo

# MEMORY_WRITER
MEMORY_WRITER_CUS ?= 4
MEMORY_WRITER_KERNEL_NAME := memory_writer
MEMORY_WRITER_CPP := $(MEMORY_WRITER_KERNEL_NAME).cpp
MEMORY_WRITER_XO := $(BUILD_DIR)/$(MEMORY_WRITER_KERNEL_NAME).xo


# HOST
CXX = g++
CXX_FLAGS = -std=c++14 -I$(FSPX_INCLUDE) -I$(VIVADO_INCLUDE) -I$(XRT_INCLUDE)
CXX_FLAGS += -Wall -Wextra -fmessage-length=0 -Wno-unknown-pragmas -Wno-unused-label -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable
# CXX_FLAGS += -g
CXX_FLAGS += -O3
# CXX_FLAGS += -fsanitize=address
# CXX_FLAGS += -fsanitize=thread
# CXX_FLAGS += -fno-omit-frame-pointer -fsanitize-address-use-after-scope -fsanitize=undefined -fno-sanitize-recover=all -fstack-protector -fstack-clash-protection -fPIE -pie -Wl,-z,relro,-z,now -Wl,-z,noexecstack -Wl,-z,defs -Wl,-z,combreloc
CXX_LDFLAGS = -L$(XRT_LIB) -L$(XRT_LIB_O) $(CXX_LIBS)
CXX_INCLUDES = -I. -I$(VITIS_INCLUDE)

host: main.cpp
	$(CXX) $(CXX_FLAGS) $(CXX_INCLUDES) -o main $< $(CXX_LDFLAGS)

emconfig:emconfig.json
emconfig.json:
	emconfigutil --platform $(PLATFORM)

$(MEMORY_READER_XO): $(MEMORY_READER_CPP)
	$(VPP) --kernel $(MEMORY_READER_KERNEL_NAME) $(VPP_FLAGS) $(VPP_INCLUDES) --connectivity.nk memory_reader:$(MEMORY_READER_CUS) -c -o $@ $<

$(COMPUTE_XO): $(COMPUTE_CPP)
	$(VPP) --kernel $(COMPUTE_KERNEL_NAME) $(VPP_FLAGS) $(VPP_INCLUDES) -c -o $@ $<

$(MEMORY_WRITER_XO): $(MEMORY_WRITER_CPP)
	$(VPP) --kernel $(MEMORY_WRITER_KERNEL_NAME) $(VPP_FLAGS) $(VPP_INCLUDES) --connectivity.nk memory_writer:$(MEMORY_WRITER_CUS) -c -o $@ $<

$(KERNEL_XCLBIN): $(MEMORY_READER_XO) $(COMPUTE_XO) $(MEMORY_WRITER_XO)
	$(VPP) --link --connectivity.nk memory_reader:$(MEMORY_READER_CUS) --connectivity.nk memory_writer:$(MEMORY_WRITER_CUS) $(VPP_FLAGS) $(VPP_LINK_FLAGS) -o $(KERNEL_LINK_XCLBIN) $(+)
	$(VPP) --package $(VPP_PACKAGE_FLAGS) $(KERNEL_LINK_XCLBIN) $(VPP_FLAGS) -o $(KERNEL_XCLBIN)


all: emconfig $(KERNEL_XCLBIN) host

clean:
	$(RM) -rf _x .run .Xil *.ltx *.log *.info *compile_summary* vitis_analyzer* *link_summary* *.o main

cleanall:
	$(RM) -rf sd_host *.o *.xo _x .run .Xil *.xclbin *.ltx *.log *.info *compile_summary* vitis_analyzer* *link_summary* *.o main emconfig.json device_trace_0.csv *.xclbin.package_summary opencl_trace.csv xrt.run_summary summary.csv .ipcache waveform_prefs.json
