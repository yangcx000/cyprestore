#
# Copyright 2020 JDD authors.
#

CYPRESTORE_TEST_DIR := $(abspath $(CURDIR))
CYPRESTORE_NBD_DIR := $(abspath $(CURDIR)/../)
CYPRESTORE_ROOT_DIR := $(abspath $(CYPRESTORE_TEST_DIR)/../../)

# Sources
SRCS_TEST = $(wildcard $(CYPRESTORE_TEST_DIR)/*.cpp)
SRCS_TEST += $(wildcard $(CYPRESTORE_NBD_DIR)/src/NBDServer.cpp)
SRCS_TEST += $(wildcard $(CYPRESTORE_NBD_DIR)/src/SafeIO.cpp)
SRCS_TEST += $(wildcard $(CYPRESTORE_NBD_DIR)/src/BlobInstance.cpp)
SRCS_TEST += $(wildcard $(CYPRESTORE_NBD_DIR)/src/util.cpp)
SRCS_TEST += $(wildcard $(CYPRESTORE_NBD_DIR)/src/argparse.cpp)

# for sdk source
CXXFLAGS += -I$(CYPRESTORE_TEST_DIR)  -g -rdynamic -O0 -g3  -D_REENTRANT -fsanitize=address -fno-omit-frame-pointer
CXXFLAGS += -I$(CYPRESTORE_ROOT_DIR)/nbd/ -I$(CYPRESTORE_ROOT_DIR)/clients/libcypre -I/usr/include/libnl3/  

# link flags 
LDFLAGS += -fsanitize=address -fno-omit-frame-pointer

# Objs
OBJS = $(SRCS_TEST:.cpp=.o)
OBJS += $(SRCS_PB:.cc=.o)

LIBS += -L$(CYPRESTORE_ROOT_DIR)/lib
LIBS += -lboost_thread -lcypre_client_static -lgtest

APP = cypre_nbd_test
include $(CYPRESTORE_ROOT_DIR)/common.mk

clean :
	rm -f $(CYPRESTORE_TEST_DIR)/*.o
	rm -f $(CYPRESTORE_NBD_DIR)/src/*.o
	rm -f $(CYPRESTORE_TEST_DIR)/$(APP)

