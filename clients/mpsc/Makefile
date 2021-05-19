#
# Copyright 2020 JDD authors.
# @yangchunxin3
#

CYPRESTORE_ROOT_DIR := $(abspath ($CURDIR)/../../..)

# Target
APP = mpsc

# Sources
SRCS_MPSC = $(wildcard $(CYPRESTORE_ROOT_DIR)/clients/mpsc/*.cpp) \
            $(CYPRESTORE_ROOT_DIR)/clients/common/ring.cpp
            #$(CYPRESTORE_ROOT_DIR)/src/common/cypre_ring.cpp \
            #$(CYPRESTORE_ROOT_DIR)/src/extentserver/spdk_mgr.cpp

# Objs
OBJS = $(SRCS_MPSC:.cpp=.o)

CXXFLAGS += -I$(CYPRESTORE_ROOT_DIR)/clients
            #-I$(CYPRESTORE_ROOT_DIR)/src/common \
            #-I$(CYPRESTORE_ROOT_DIR)/src/extentserver

include $(CYPRESTORE_ROOT_DIR)/common.mk

CXXFLAGS += -I$(DPDK_HEADER_DIR)
CXXFLAGS += -I$(SPDK_HEADER_DIR)

# SPDK lib names
SPDK_LIB_NAMES += bdev_nvme bdev_error bdev_aio event_bdev event_accel event_vmd vmd nvme
SPDK_LIB_NAMES += bdev_rpc bdev accel event thread util conf trace log jsonrpc json rpc sock notify
SPDK_LIB_NAMES += env_dpdk  

DPDK_LIB_NAMES = rte_eal rte_mempool rte_ring rte_mbuf rte_mempool_ring rte_pci rte_bus_pci rte_kvargs

LIBS += -L${SPDK_LIB_DIR} -Wl,--whole-archive -Wl,--no-as-needed $(SPDK_LIB_NAMES:%=-lspdk_%) -Wl,--no-whole-archive

LIBS += -L${DPDK_LIB_DIR} -Wl,--whole-archive -Wl,--no-as-needed $(DPDK_LIB_NAMES:%=-l%) -Wl,--no-whole-archive

clean :
	rm -f $(CYPRESTORE_ROOT_DIR)/clients/mpsc/$(APP)
	rm -f $(CYPRESTORE_ROOT_DIR)/clients/mpsc/*.o
