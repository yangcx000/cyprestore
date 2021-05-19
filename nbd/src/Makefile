#
# Copyright 2020 JDD authors.
# @yangbing1
#

PROJECT_DIR = $(CURDIR)
CYPRESTORE_ROOT_DIR := $(abspath $(CURDIR)/../../)
GIT_COMMIT_ID := $(shell git rev-parse HEAD)

# Sources
SOURCES = $(wildcard $(PROJECT_DIR)/*.cpp) \
	  $(wildcard $(PROJECT_DIR)/public/*.cpp) \
	  $(wildcard $(CYPRESTORE_ROOT_DIR)/src/common/log.cpp) \
 
# GCC compile flags
CXXFLAGS += -rdynamic  -I$(CYPRESTORE_ROOT_DIR)/clients/libcypre -I$(CYPRESTORE_ROOT_DIR)/clients -I$(PROJECT_DIR)/public  -I/usr/include/libnl3/  -DGIT_COMMIT_ID=\"$(GIT_COMMIT_ID)\"
#-DDODBG -fsanitize=address -fno-omit-frame-pointer

# GCC link flags: Relocation ReadOnly, Disable lazy binding, Stack execution protection
LDFLAGS += -L$(CYPRESTORE_ROOT_DIR)/lib   -L$(CYPRESTORE_ROOT_DIR)/third-party/tcmalloc/lib 
#-fsanitize=address

# Objs
OBJS = $(SOURCES:.cpp=.o) 


LIBS += -lboost_system -lboost_thread -lnl-3 -lnl-genl-3  -lcypre_client_static -lbrpc  -ltcmalloc_static 
# -lcypre_client_static  

APP = cypre_nbd
include $(CYPRESTORE_ROOT_DIR)/common.mk
 

clean:
	rm -f $(PROJECT_DIR)/*.o  	
	rm -f $(PROJECT_DIR)/public/*.o  
	


 
