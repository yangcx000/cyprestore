#
# Copyright 2020 JDD authors.
# @yangchunxin3
#

CYPRESTORE_ROOT_DIR := $(abspath ($CURDIR)/../../..)

# Target
APP = extentmanager

# Sources
SRCS_EXTENTMANAGER = $(wildcard $(CYPRESTORE_ROOT_DIR)/src/extentmanager/*.cpp)
SRCS_KV_STORE = $(wildcard $(CYPRESTORE_ROOT_DIR)/src/kvstore/*.cpp)

SRCS_COMMON = $(CYPRESTORE_ROOT_DIR)/src/common/config.cpp \
			  $(CYPRESTORE_ROOT_DIR)/src/common/constants.cpp \
			  $(CYPRESTORE_ROOT_DIR)/src/common/log.cpp

SRCS_UTILS = $(CYPRESTORE_ROOT_DIR)/src/utils/chrono.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/ini_parser.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/pb_transfer.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/uuid.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/timer_thread.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/crc32.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/hash.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/coding.cpp \
			 $(CYPRESTORE_ROOT_DIR)/src/utils/gcd_util.cpp

SRCS_KVSTORE = $(CYPRESTORE_ROOT_DIR)/src/kvstore/rocks_store.cpp

SRCS_PB += $(wildcard $(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/*.cc)
SRCS_PB += $(wildcard $(CYPRESTORE_ROOT_DIR)/src/common/pb/*.cc)
SRCS_PB += $(CYPRESTORE_ROOT_DIR)/src/setmanager/pb/heartbeat.pb.cc
SRCS_PB += $(CYPRESTORE_ROOT_DIR)/src/extentserver/pb/extent_io.pb.cc
SRCS_PB += $(CYPRESTORE_ROOT_DIR)/src/extentserver/pb/extent_control.pb.cc

# Objs
OBJS += $(SRCS_EXTENTMANAGER:.cpp=.o)
OBJS += $(SRCS_KV_STORE:.cpp=.o)
OBJS += $(SRCS_COMMON:.cpp=.o)
OBJS += $(SRCS_UTILS:.cpp=.o)
OBJS += $(SRCS_PB:.cc=.o)

include $(CYPRESTORE_ROOT_DIR)/common.mk

CXXFLAGS += -I$(ROCKSDB_HEADER_DIR)

LIBS += -L${ROCKSDB_LIB_DIR} -lrocksdb
LIBS += -lsnappy -lz -lbz2 -llz4 -lboost_thread

clean :
	rm -f $(CYPRESTORE_ROOT_DIR)/src/utils/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/common/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/common/pb/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/setmanager/pb/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/kvstore/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/$(APP)/$(APP)
	rm -f $(CYPRESTORE_ROOT_DIR)/src/$(APP)/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/$(APP)/pb/*.o
