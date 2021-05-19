#
# Copyright 2020 JDD authors.
# @yangchunxin3
# 1. make all
# 2. make fio FIO_PATH=/home/dev/chenjianfei/fio-3.15/fio-fio-3.15
#

CYPRESTORE_ROOT_DIR := $(abspath ($CURDIR)/../../..)

# Target
APP = c_test

# Sources
SRCS_CYPRECLIENT = $(wildcard $(CYPRESTORE_ROOT_DIR)/clients/fio_plugin/*.cpp)

# Objs
OBJS = $(SRCS_CYPRECLIENT:.cpp=.o)
OBJS += $(SRCS_PB:.cc=.o)

LIBS += -L$(CYPRESTORE_ROOT_DIR)/lib
LIBS += -lboost_thread -lcypre_client_static

CXXFLAGS += -I$(CYPRESTORE_ROOT_DIR)/clients -I$(CYPRESTORE_ROOT_DIR)/clients/libcypre

include $(CYPRESTORE_ROOT_DIR)/common.mk

clean :
	rm -f $(CYPRESTORE_ROOT_DIR)/clients/fio_plugin/$(APP)
	rm -f $(CYPRESTORE_ROOT_DIR)/clients/fio_plugin/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/clients/fio_plugin/*.tmp
	rm -f $(CYPRESTORE_ROOT_DIR)/clients/fio_plugin/fio_plugin

fio:
	gcc -g -O2 -o fio_plugin.o -MMD -MP -MF fio_plugin.d.tmp -I$(FIO_PATH) -g  -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -fno-strict-aliasing -march=native -Wformat -Wformat-security -D_GNU_SOURCE -fPIC -fstack-protector -fno-common -DNDEBUG -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -pthread -Wno-pointer-sign -Wstrict-prototypes -Wold-style-definition -std=gnu99 -c fio_plugin.c
	gcc -g -O2 -o fio_plugin  -shared -rdynamic -Wl,-z,relro,-z,now -Wl,-z,noexecstack -fuse-ld=bfd -pthread fio_plugin.o  $(CYPRESTORE_ROOT_DIR)/lib/libcypre_client_static.a $(BRPC_LIB_DIR)/libbrpc.a -Wl,--no-whole-archive -lboost_thread -pthread -lnuma -lgflags -lleveldb -lprotobuf -ldl -lrt -luuid -lssl -lcrypto -laio -ltcmalloc
