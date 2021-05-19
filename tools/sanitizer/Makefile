#
# Copyright 2020 JDD authors.
# @yangchunxin3
#

CYPRESTORE_ROOT_DIR := $(abspath ($CURDIR)/../../..)

# Target
APP = sanitizer

# Sources
SRCS_SANITIZER = $(wildcard $(CYPRESTORE_ROOT_DIR)/tools/sanitizer/*.cpp)

# Objs
OBJS = $(SRCS_SANITIZER:.cpp=.o)

CXXFLAGS += -I$(CYPRESTORE_ROOT_DIR)/tools

include $(CYPRESTORE_ROOT_DIR)/common.mk

clean :
	rm -f $(CYPRESTORE_ROOT_DIR)/tools/sanitizer/$(APP)
	rm -f $(CYPRESTORE_ROOT_DIR)/tools/sanitizer/*.o
