#
# Copyright 2020 JDD authors.
# @yangchunxin3
#

CYPRESTORE_ROOT_DIR := $(abspath ($CURDIR)/../../..)

APP = cypreadmin

# src files
SRCS_EXTENTMANAGER_PB = $(wildcard $(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/*.cc)
SRCS_COMMON_PB = $(wildcard $(CYPRESTORE_ROOT_DIR)/src/common/pb/*.cc)
SRCS_CYPREADMIN = $(wildcard $(CYPRESTORE_ROOT_DIR)/tools/cypreadmin/*.cpp)

# objs
OBJS += $(SRCS_EXTENTMANAGER_PB:.cc=.o)
OBJS += $(SRCS_COMMON_PB:.cc=.o)
OBJS += $(SRCS_CYPREADMIN:.cpp=.o)

include $(CYPRESTORE_ROOT_DIR)/common.mk

clean :
	rm -f $(CYPRESTORE_ROOT_DIR)/src/common/pb/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/tools/$(APP)/$(APP)
	rm -f $(CYPRESTORE_ROOT_DIR)/tools/$(APP)/*.o
