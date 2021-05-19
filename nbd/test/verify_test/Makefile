#
# Copyright 2020 JDD authors.
# @yangbing1
#

PROJECT_DIR = $(CURDIR)
BRPC_ROOT_DIR = $(abspath ${PROJECT_DIR}/../../incubator-brpc)

# Target
APP = verify_test

# Compiler
CXX = c++

# GCC compile flags
CXXFLAGS = -g -O2 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -Wformat -Wformat-security
CXXFLAGS += -D_GNU_SOURCE -DNDEBUG -D_FORTIFY_SOURCE=2 -U_FORTIFY_SOURCE -DGFLAGS_NS=gflags
CXXFLAGS += -fPIC -fstack-protector -fno-strict-aliasing -fno-common
CXXFLAGS += -std=c++11 -pthread -march=native
#CXXFLAGS += -I$(BRPC_ROOT_DIR)/output/include -I$(PROJECT_DIR)/src -I$(PROJECT_DIR)/tools/public  -I/usr/include/libnl3/ -I$(CURDIR)/

# GCC link flags: Relocation ReadOnly, Disable lazy binding, Stack execution protection
LDFLAGS += -Wl,-z,relro,-z,now -Wl,-z,noexecstack
#LDFLAGS += -pthread -lnuma -lgflags -lleveldb -lprotobuf

# Dependency libs
DependencyLibs = -lrt -ldl 
#-lssl -lcrypto -laio -lbrpc -lboost_system -lnl-3 -lnl-genl-3

# BRPC lib
BRPC_LIB_LINKER_ARGS = 
#-L$(BRPC_ROOT_DIR)/output/lib

# Source files
SOURCES = $(wildcard $(CURDIR)/*.cpp)  

# Objs
OBJS = $(SOURCES:.cpp=.o) 

# Compile rule
COMPILE_CXX=\
	echo "  CXX $S/$@"; \
	$(CXX) -o $@ $(CXXFLAGS) -c $<

# Link rule
LINK_CXX=\
	echo "  LINK $S/$@"; \
	$(CXX) -o $(PROJECT_DIR)/$(APP) $(LDFLAGS) $(OBJS) $(BRPC_LIB_LINKER_ARGS) $(DependencyLibs)

# Default rule
all : $(APP)
	@:

$(APP) : $(OBJS)
	$(LINK_CXX)

%.o : %.cpp %.cc
	$(COMPILE_CXX)

install : all

.PHONY : clean

clean :
	rm -f $(PROJECT_DIR)/*.o 
