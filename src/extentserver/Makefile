# Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file. See the AUTHORS file for names of contributors.

CYPRESTORE_ROOT_DIR := $(abspath ($CURDIR)/../../..)

# Target
APP = extentserver

# Sources
SRCS_EXTENTSERVER = $(wildcard $(CYPRESTORE_ROOT_DIR)/src/extentserver/*.cpp)
SRCS_KV_STORE = $(wildcard $(CYPRESTORE_ROOT_DIR)/src/kvstore/rocks_store.cpp)

SRCS_COMMON = $(CYPRESTORE_ROOT_DIR)/src/common/config.cpp \
              $(CYPRESTORE_ROOT_DIR)/src/common/constants.cpp \
              $(CYPRESTORE_ROOT_DIR)/src/common/extent_router.cpp \
              $(CYPRESTORE_ROOT_DIR)/src/common/arena.cpp \
              $(CYPRESTORE_ROOT_DIR)/src/common/cypre_ring.cpp \
              $(CYPRESTORE_ROOT_DIR)/src/common/log.cpp

SRCS_UTILS = $(CYPRESTORE_ROOT_DIR)/src/utils/chrono.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/ini_parser.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/timer_thread.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/tools.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/uuid.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/crc32.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/coding.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/set_cpu_affinity.cpp

SRCS_PB += $(wildcard $(CYPRESTORE_ROOT_DIR)/src/extentserver/pb/*.cc)
SRCS_PB += $(wildcard $(CYPRESTORE_ROOT_DIR)/src/common/pb/*.cc)
SRCS_PB += $(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/heartbeat.pb.cc \
           $(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/router.pb.cc

# Objs
OBJS = $(SRCS_KV_STORE:.cpp=.o)
OBJS += $(SRCS_COMMON:.cpp=.o)
OBJS += $(SRCS_UTILS:.cpp=.o)
OBJS += $(SRCS_PB:.cc=.o)
OBJS += $(SRCS_EXTENTSERVER:.cpp=.o)

include $(CYPRESTORE_ROOT_DIR)/common.mk

CXXFLAGS += -I$(ROCKSDB_HEADER_DIR)
CXXFLAGS += -I$(SPDK_HEADER_DIR)
CXXFLAGS += -I$(DPDK_HEADER_DIR)

# SPDK lib names
SPDK_LIB_NAMES += bdev_nvme bdev_error bdev_aio event_bdev event_accel event_vmd vmd nvme
SPDK_LIB_NAMES += bdev_rpc bdev accel event thread util conf trace log jsonrpc json rpc sock notify
SPDK_LIB_NAMES += env_dpdk  

# DPDK lib names
DPDK_LIB_NAMES = rte_eal rte_mempool rte_ring rte_mbuf rte_mempool_ring rte_pci rte_bus_pci rte_kvargs

LIBS += -L${SPDK_LIB_DIR} -Wl,--whole-archive -Wl,--no-as-needed $(SPDK_LIB_NAMES:%=-lspdk_%) -Wl,--no-whole-archive
LIBS += -L${DPDK_LIB_DIR} -Wl,--whole-archive -Wl,--no-as-needed $(DPDK_LIB_NAMES:%=-l%) -Wl,--no-whole-archive
LIBS += -lsnappy -lz -llz4 -lbz2 -lboost_thread
LIBS += -L${ROCKSDB_LIB_DIR} -lrocksdb

clean :
	rm -f $(CYPRESTORE_ROOT_DIR)/src/utils/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/common/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/common/pb/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/kvstore/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/$(APP)/$(APP)
	rm -f $(CYPRESTORE_ROOT_DIR)/src/$(APP)/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/$(APP)/pb/*.o
