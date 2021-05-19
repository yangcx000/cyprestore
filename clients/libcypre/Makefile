#
# Copyright 2020 JDD authors.
#

CYPRESTORE_SDK_DIR := $(abspath $(CURDIR))
CYPRESTORE_ROOT_DIR := $(abspath $(CYPRESTORE_SDK_DIR)/../../)
# Sources
SRCS_CYPRECLIENT = $(wildcard $(CYPRESTORE_SDK_DIR)/*.cpp) \
		$(wildcard $(CYPRESTORE_SDK_DIR)/stream/*.cpp) \
		$(wildcard $(CYPRESTORE_SDK_DIR)/concurrency/*.cpp) \
		$(wildcard $(CYPRESTORE_SDK_DIR)/mock/*.cpp) \
		$(CYPRESTORE_ROOT_DIR)/clients/common/ring.cpp \
		$(CYPRESTORE_ROOT_DIR)/src/utils/chrono.cpp \
		$(CYPRESTORE_ROOT_DIR)/src/utils/crc32.cpp \
		$(CYPRESTORE_ROOT_DIR)/src/utils/pb_transfer.cpp \
		$(CYPRESTORE_ROOT_DIR)/src/common/connection_pool.cpp \
		$(CYPRESTORE_ROOT_DIR)/src/common/extent_router.cpp

SRCS_PB += $(CYPRESTORE_ROOT_DIR)/src/extentserver/pb/extent_io.pb.cc
SRCS_PB += $(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/resource.pb.cc \
           $(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/router.pb.cc
SRCS_PB += $(wildcard $(CYPRESTORE_ROOT_DIR)/src/common/pb/*.cc)

# for sdk source
CXXFLAGS += -I$(CYPRESTORE_SDK_DIR) -I$(CYPRESTORE_SDK_DIR)/../

# Objs
OBJS = $(SRCS_CYPRECLIENT:.cpp=.o)
OBJS += $(SRCS_PB:.cc=.o)


LIBS += -lboost_thread

APP = libcypre_client
include $(CYPRESTORE_ROOT_DIR)/common.mk

clean:
	rm -f $(CYPRESTORE_ROOT_DIR)/src/common/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/common/pb/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/*.o
	rm -f $(CYPRESTORE_SDK_DIR)/*.o
	rm -f $(CYPRESTORE_SDK_DIR)/*/*.o
	rm -f $(CYPRESTORE_SDK_DIR)/*.so
	rm -f $(CYPRESTORE_SDK_DIR)/*.a

