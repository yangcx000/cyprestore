# Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file. See the AUTHORS file for names of contributors.

CYPRESTORE_ROOT_DIR := $(abspath ($CURDIR)/../../..)

APP = access

# sources
SRCS_ACCESS = $(wildcard $(CYPRESTORE_ROOT_DIR)/src/access/*.cpp)
SRCS_COMMON = $(CYPRESTORE_ROOT_DIR)/src/common/config.cpp \
	          $(CYPRESTORE_ROOT_DIR)/src/common/log.cpp

SRCS_UTILS = $(CYPRESTORE_ROOT_DIR)/src/utils/chrono.cpp \
             $(CYPRESTORE_ROOT_DIR)/src/utils/ini_parser.cpp \
             $(CYPRESTORE_ROOT_DIR)/src/utils/regex_util.cpp \
             $(CYPRESTORE_ROOT_DIR)/src/utils/timer_thread.cpp

SRCS_PB += $(wildcard $(CYPRESTORE_ROOT_DIR)/src/access/pb/*.cc)
SRCS_PB += $(wildcard $(CYPRESTORE_ROOT_DIR)/src/setmanager/pb/*.cc)
SRCS_PB += $(wildcard $(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/*.cc)
SRCS_PB += $(wildcard $(CYPRESTORE_ROOT_DIR)/src/common/pb/*.cc)
SRCS_PB += $(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/resource.pb.cc

# objs
OBJS += $(SRCS_ACCESS:.cpp=.o)
OBJS += $(SRCS_COMMON:.cpp=.o)
OBJS += $(SRCS_UTILS:.cpp=.o)
OBJS += $(SRCS_PB:.cc=.o)

include $(CYPRESTORE_ROOT_DIR)/common.mk

LIBS += -lz -llz4 -lbz2 -lboost_regex -lboost_thread

clean :
	rm -f $(CYPRESTORE_ROOT_DIR)/src/utils/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/common/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/common/pb/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/setmanager/pb/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/bin/$(APP)
	rm -f $(CYPRESTORE_ROOT_DIR)/src/$(APP)/$(APP)
	rm -f $(CYPRESTORE_ROOT_DIR)/src/$(APP)/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/$(APP)/pb/*.o
