#
# Copyright 2020 JDD authors.
# @yangchunxin3
#

CYPRESTORE_ROOT_DIR := $(abspath ($CURDIR)/../../..)

# Target
APP = cyprebench

# Sources
SRCS_CYPRECLIENT = $(wildcard $(CYPRESTORE_ROOT_DIR)/clients/cyprebench/*.cpp) \
				   $(CYPRESTORE_ROOT_DIR)/clients/common/ring.cpp \
				   $(CYPRESTORE_ROOT_DIR)/src/utils/chrono.cpp

# Objs
OBJS = $(SRCS_CYPRECLIENT:.cpp=.o)
OBJS += $(SRCS_PB:.cc=.o)

LIBS += -L$(TCMALLOC_LIB_DIR)
LIBS += -L$(CYPRESTORE_ROOT_DIR)/lib
LIBS += -lboost_thread -lcypre_client_static -ltcmalloc_static

CXXFLAGS += -I$(CYPRESTORE_ROOT_DIR)/clients \
		    -I$(CYPRESTORE_ROOT_DIR)/clients/libcypre \
            -I$(CYPRESTORE_ROOT_DIR)/src/common \
            -I$(CYPRESTORE_ROOT_DIR)/src/utils

include $(CYPRESTORE_ROOT_DIR)/common.mk

clean :
	rm -f $(CYPRESTORE_ROOT_DIR)/clients/cyprebench/$(APP)
	rm -f $(CYPRESTORE_ROOT_DIR)/clients/cyprebench/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/clients/common/*.o
