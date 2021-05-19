#
# Copyright 2020 JDD authors.
# @yangchunxin3
#

CYPRESTORE_ROOT_DIR := $(abspath ($CURDIR)/../../../..)

# Target
APP = spdk_test

# Sources
SRCS_SPDK_TEST = $(wildcard $(CYPRESTORE_ROOT_DIR)/src/extentserver/spdk_test/*.cpp) \
				 $(wildcard $(CYPRESTORE_ROOT_DIR)/src/extentserver/spdk_mgr.cpp) \
				 $(wildcard $(CYPRESTORE_ROOT_DIR)/src/extentserver/io_mem.cpp) \
				 $(wildcard $(CYPRESTORE_ROOT_DIR)/src/extentserver/arena.cpp) \
				 $(wildcard $(CYPRESTORE_ROOT_DIR)/src/extentserver/ring.cpp) \
				 $(wildcard $(CYPRESTORE_ROOT_DIR)/src/extentserver/bitmap_allocator.cpp)

SRCS_COMMON = $(CYPRESTORE_ROOT_DIR)/src/common/config.cpp \
              $(CYPRESTORE_ROOT_DIR)/src/common/constants.cpp \
              $(CYPRESTORE_ROOT_DIR)/src/common/log.cpp

SRCS_UTILS = $(CYPRESTORE_ROOT_DIR)/src/utils/chrono.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/ini_parser.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/timer_thread.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/tools.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/uuid.cpp

# Objs
OBJS = $(SRCS_SPDK_TEST:.cpp=.o)
OBJS += $(SRCS_COMMON:.cpp=.o)
OBJS += $(SRCS_UTILS:.cpp=.o)

CXXFLAGS += -I$(CYPRESTORE_ROOT_DIR)/third-party/spdk/include
CXXFLAGS += -I$(CYPRESTORE_ROOT_DIR)/third-party/spdk/dpdk/build/include

include $(CYPRESTORE_ROOT_DIR)/common.mk

CXXFLAGS += -I$(SPDK_HEADER_DIR)

# SPDK lib names
SPDK_LIB_NAMES += bdev_nvme bdev_error bdev_aio event_bdev event_accel event_vmd vmd nvme
SPDK_LIB_NAMES += bdev_rpc bdev accel event thread util conf trace log jsonrpc json rpc sock notify
SPDK_LIB_NAMES += env_dpdk  

# DPDK lib names
DPDK_LIB_NAMES = rte_eal rte_mempool rte_ring rte_mbuf rte_mempool_ring rte_pci rte_bus_pci rte_kvargs

LIBS += -L${SPDK_LIB_DIR} -Wl,--whole-archive -Wl,--no-as-needed $(SPDK_LIB_NAMES:%=-lspdk_%) -Wl,--no-whole-archive
LIBS += -L${DPDK_LIB_DIR} -Wl,--whole-archive -Wl,--no-as-needed $(DPDK_LIB_NAMES:%=-l%) -Wl,--no-whole-archive
LIBS += -lsnappy -lz -llz4 -lbz2

clean :
	rm -f $(CYPRESTORE_ROOT_DIR)/src/utils/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/common/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentserver/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentserver/$(APP)/$(APP)
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentserver/$(APP)/*.o
