#
# Copyright 2020 JDD authors.
#

CYPRESTORE_TEST_DIR := $(abspath $(CURDIR))
CYPRESTORE_ROOT_DIR := $(abspath $(CYPRESTORE_TEST_DIR)/../../../)
# Sources
SRCS_TEST = $(wildcard $(CYPRESTORE_TEST_DIR)/*.cpp)

# for sdk source
CXXFLAGS += -I$(CYPRESTORE_SDK_DIR)
CXXFLAGS += -I$(CYPRESTORE_ROOT_DIR)/clients -I$(CYPRESTORE_ROOT_DIR)/clients/libcypre

# Objs
OBJS = $(SRCS_TEST:.cpp=.o)
OBJS += $(SRCS_PB:.cc=.o)

LIBS += -L$(CYPRESTORE_ROOT_DIR)/lib
LIBS += -lboost_thread -lcypre_client_static

APP = mock_es
include $(CYPRESTORE_ROOT_DIR)/common.mk

clean :
	rm -f $(CYPRESTORE_TEST_DIR)/*.o
	rm -f $(CYPRESTORE_TEST_DIR)/$(APP)

