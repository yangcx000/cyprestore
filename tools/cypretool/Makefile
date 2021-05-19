#
# Copyright 2020 JDD authors.
# @yangchunxin3
#

CYPRESTORE_ROOT_DIR := $(abspath ($CURDIR)/../../..)

APP = cypretool

# src files
SRCS_ACCESS_PB = $(wildcard $(CYPRESTORE_ROOT_DIR)/src/access/pb/*.cc)
SRCS_EXTENTMANAGER_PB = $(wildcard $(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/*.cc)
SRCS_EXTENTSERVER_PB = $(wildcard $(CYPRESTORE_ROOT_DIR)/src/extentserver/pb/*.cc)
SRCS_COMMON_PB = $(wildcard $(CYPRESTORE_ROOT_DIR)/src/common/pb/*.cc)
SRCS_CYPRETOOL = $(wildcard $(CYPRESTORE_ROOT_DIR)/tools/cypretool/*.cpp)

# objs
OBJS += $(SRCS_ACCESS_PB:.cc=.o)
OBJS += $(SRCS_EXTENTMANAGER_PB:.cc=.o)
OBJS += $(SRCS_EXTENTSERVER_PB:.cc=.o)
OBJS += $(SRCS_COMMON_PB:.cc=.o)
OBJS += $(SRCS_CYPRETOOL:.cpp=.o)

include $(CYPRESTORE_ROOT_DIR)/common.mk

LIBS += -lboost_thread

clean :
	rm -f $(CYPRESTORE_ROOT_DIR)/src/common/pb/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/access/pb/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentserver/pb/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/tools/$(APP)/$(APP)
	rm -f $(CYPRESTORE_ROOT_DIR)/tools/$(APP)/*.o
