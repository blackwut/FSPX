# INPUTS
KERNEL_NAME ?= sd
MR_PAR ?= 1
MA_PAR ?= 1
SD_PAR ?= 1
MW_PAR ?= 1


################################################################################
#
# Environment
#
################################################################################
FSPX_INCLUDE := $(HOME)/FSPX_blackwut/include
VITIS_INCLUDE := $(XILINX_VITIS)/include
VIVADO_INCLUDE := $(XILINX_VIVADO)/include
XRT_INCLUDE := $(XILINX_XRT)/include


################################################################################
#
# Libraries
#
################################################################################
XRT_LIB := $(XILINX_XRT)/lib
XRT_LIB_O := $(XRT_LIB)lnx64.o
CXX_LIBS := -lOpenCL -pthread -lrt -lstdc++


################################################################################
#
# Target, Device, Platform
#
################################################################################
TARGET ?= sw_emu
DEVICE ?= u50
PLATFORM ?= xilinx_$(DEVICE)_gen3x16_xdma_5_202210_1
PLATFORM_REPO_PATHS ?= /opt/xilinx/platforms
PFM := $(PLATFORM_REPO_PATHS)/$(PLATFORM)/$(PLATFORM).xpfm


################################################################################
#
# Directories
#
################################################################################
KERNELS_DIR := ./kernels
KERNEL_DIR := ${KERNEL_NAME}$(MR_PAR)$(MA_PAR)$(SD_PAR)$(MW_PAR)
OUT_DIR = $(KERNELS_DIR)/${KERNEL_DIR}/$(TARGET)
TEMP_DIR := $(OUT_DIR)/temp
LOG_DIR := $(OUT_DIR)/log
REPORT_DIR := $(OUT_DIR)/report
BUILD_DIR := $(OUT_DIR)/build

################################################################################
#
# Vitis Compiler (v++)
#
################################################################################
VPP = v++
VPP_INCLUDES := -I. -Idevice -Icommon -Ioperators -Ituples -I$(FSPX_INCLUDE)
VPP_FLAGS := --platform $(PFM) --target $(TARGET)
VPP_FLAGS += --debug
VPP_FLAGS += --save-temps
VPP_FLAGS += --temp_dir $(TEMP_DIR)
VPP_FLAGS += --log_dir $(LOG_DIR)
VPP_FLAGS += --report_dir $(REPORT_DIR)
VPP_PACKAGE_FLAGS := --package.out_dir $(BUILD_DIR)


################################################################################
#
# Kernel names and source files
#
################################################################################
# MEMORY_READER
MEMORY_READER_KERNEL_NAME := memory_reader
MEMORY_READER_CPP := device/$(MEMORY_READER_KERNEL_NAME).cpp
MEMORY_READER_XO := $(BUILD_DIR)/$(MEMORY_READER_KERNEL_NAME).xo

# COMPUTE
COMPUTE_KERNEL_NAME := compute
COMPUTE_CPP := device/$(COMPUTE_KERNEL_NAME).cpp
COMPUTE_XO := $(BUILD_DIR)/$(COMPUTE_KERNEL_NAME).xo

# MEMORY_WRITER
MEMORY_WRITER_KERNEL_NAME := memory_writer
MEMORY_WRITER_CPP := device/$(MEMORY_WRITER_KERNEL_NAME).cpp
MEMORY_WRITER_XO := $(BUILD_DIR)/$(MEMORY_WRITER_KERNEL_NAME).xo

# BITSTREAM
KERNEL_LINK_XCLBIN := $(BUILD_DIR)/$(KERNEL_NAME).link.xclbin
KERNEL_XCLBIN := $(BUILD_DIR)/$(KERNEL_NAME).xclbin


################################################################################
#
# Compute Units (nk), Stream Channels (sc) and Stream Ports (sp)
#
################################################################################
# MEMORY_READER
MR_SCRIPT := $(FSPX_INCLUDE)/utils/memory_reader.sh
MR_NK := $(shell bash -c 'source "$(MR_SCRIPT)" && generate_nk "$(MR_PAR)"')
MR_SC := $(shell bash -c 'source "$(MR_SCRIPT)" && generate_sc "$(MR_PAR)"')
MR_SP := $(shell bash -c 'source "$(MR_SCRIPT)" && generate_sp "$(MR_PAR)"')
MR_COMPILE_FLAGS := $(MR_NK) $(MR_SP)
MR_LINK_FLAGS := $(MR_COMPILE_FLAGS) $(MR_SC)

# COMPUTE
COMPUTE_NK := "--connectivity.nk=$(COMPUTE_KERNEL_NAME):1:$(COMPUTE_KERNEL_NAME)_0"
COMPUTE_COMPILE_FLAGS := $(COMPUTE_NK) -DMR_PAR=$(MR_PAR) -DMA_PAR=$(MA_PAR) -DSD_PAR=$(SD_PAR) -DMW_PAR=$(MW_PAR)
COMPUTE_LINK_FLAGS := $(COMPUTE_COMPILE_FLAGS)

# MEMORY_WRITER
MW_SCRIPT := $(FSPX_INCLUDE)/utils/memory_writer.sh
MW_NK := $(shell bash -c 'source "$(MW_SCRIPT)" && generate_nk "$(MW_PAR)"')
MW_SC := $(shell bash -c 'source "$(MW_SCRIPT)" && generate_sc "$(MW_PAR)"')
MW_SP := $(shell bash -c 'source "$(MW_SCRIPT)" && generate_sp "$(MW_PAR)"')
MW_COMPILE_FLAGS := $(MW_NK) $(MW_SP)
MW_LINK_FLAGS := $(MW_COMPILE_FLAGS) $(MW_SC)

################################################################################
#
# C++ Host
#
################################################################################
CXX = g++
CXX_FLAGS = -std=c++17
CXX_FLAGS += -Wall -Wextra -fmessage-length=0 -Wno-unknown-pragmas -Wno-unused-label -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable
CXX_FLAGS += -g
CXX_FLAGS += -O3
# CXX_FLAGS += -fsanitize=address
# CXX_FLAGS += -fsanitize=thread
# CXX_FLAGS += -fno-omit-frame-pointer -fsanitize-address-use-after-scope -fsanitize=undefined -fno-sanitize-recover=all -fstack-protector -fstack-clash-protection -fPIE -pie -Wl,-z,relro,-z,now -Wl,-z,noexecstack -Wl,-z,defs -Wl,-z,combreloc
CXX_LDFLAGS = -L$(XRT_LIB) -L$(XRT_LIB_O) $(CXX_LIBS)
CXX_INCLUDES = -Icommon -Ioperators -Ituples -I$(FSPX_INCLUDE) -I$(VIVADO_INCLUDE) -I$(XRT_INCLUDE) -I$(VITIS_INCLUDE)


host: main.cpp
	mkdir -p bin
	$(CXX) $(CXX_FLAGS) $(CXX_INCLUDES) -o bin/main $< $(CXX_LDFLAGS)

emconfig:emconfig.json
emconfig.json:
	emconfigutil --platform $(PLATFORM)

$(MEMORY_READER_XO): $(MEMORY_READER_CPP)
	$(VPP) --kernel $(MEMORY_READER_KERNEL_NAME) $(VPP_FLAGS) $(VPP_INCLUDES) $(MR_COMPILE_FLAGS) -c -o $@ $<

$(COMPUTE_XO): $(COMPUTE_CPP)
	$(VPP) --kernel $(COMPUTE_KERNEL_NAME) $(VPP_FLAGS) $(VPP_INCLUDES) $(COMPUTE_COMPILE_FLAGS) -c -o $@ $<

$(MEMORY_WRITER_XO): $(MEMORY_WRITER_CPP)
	$(VPP) --kernel $(MEMORY_WRITER_KERNEL_NAME) $(VPP_FLAGS) $(VPP_INCLUDES) $(MW_COMPILE_FLAGS) -c -o $@ $<

$(KERNEL_LINK_XCLBIN): $(MEMORY_READER_XO) $(COMPUTE_XO) $(MEMORY_WRITER_XO)
	$(VPP) --link $(VPP_FLAGS) $(VPP_LINK_FLAGS) $(MR_LINK_FLAGS) $(COMPUTE_LINK_FLAGS) $(MW_LINK_FLAGS) -o $(KERNEL_LINK_XCLBIN) $(+)

bitstream: $(KERNEL_LINK_XCLBIN)
	$(VPP) --package $(VPP_PACKAGE_FLAGS) $(KERNEL_LINK_XCLBIN) $(VPP_FLAGS) -o $(KERNEL_XCLBIN)


all: emconfig $(KERNEL_XCLBIN) host

clean:
	$(RM) -rf _x .run .Xil *.ltx *.log *.info *compile_summary* vitis_analyzer* *link_summary* *.o main

cleanall:
	$(RM) -rf sd_host *.o *.xo _x .run .Xil *.xclbin *.ltx *.log *.info *compile_summary* vitis_analyzer* *link_summary* *.o main emconfig.json device_trace_0.csv *.xclbin.package_summary opencl_trace.csv xrt.run_summary summary.csv .ipcache waveform_prefs.json
