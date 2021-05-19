#
# Copyright 2020 JDD authors.
# @yangbing1
#

PROJECT_DIR = $(CURDIR)/../..
BRPC_ROOT_DIR = $(abspath ${PROJECT_DIR}/../../incubator-brpc)

# Target
APP = cypreclient
 

# Compiler
CXX = c++

# GCC compile flags
CXXFLAGS = -g -O2 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -Wformat -Wformat-security
CXXFLAGS += -D_GNU_SOURCE -DNDEBUG -D_FORTIFY_SOURCE=2 -U_FORTIFY_SOURCE -DGFLAGS_NS=gflags
CXXFLAGS += -fPIC -fstack-protector -fno-strict-aliasing -fno-common
CXXFLAGS += -std=c++11 -pthread -march=native
CXXFLAGS += -I$(BRPC_ROOT_DIR)/output/include -I$(PROJECT_DIR)/src  -I$(PROJECT_DIR)/src/utils -I$(PROJECT_DIR)/tools/public

# GCC link flags: Relocation ReadOnly, Disable lazy binding, Stack execution protection
LDFLAGS += -Wl,-z,relro,-z,now -Wl,-z,noexecstack
LDFLAGS += -pthread -lnuma -lgflags -lleveldb -lprotobuf

# Dependency libs
DependencyLibs = -lrt -ldl -luuid -lssl -lcrypto -laio -lbrpc -lboost_system 

# BRPC lib
BRPC_LIB_LINKER_ARGS = -L$(BRPC_ROOT_DIR)/output/lib

# Source files
ACCESS_PB_SRCS = $(wildcard $(PROJECT_DIR)/src/access/pb/*.cc)
EXTENTMANAGER_PB_SRCS = $(wildcard $(PROJECT_DIR)/src/extentmanager/pb/*.cc)
EXTENTSERVER_PB_SRCS = $(wildcard $(PROJECT_DIR)/src/extentserver/pb/*.cc) 
SETMANAGER_PB_SRCS += $(wildcard $(PROJECT_DIR)/src/setmanager/pb/*.cc) 
COMMON_PB_SRCS = $(wildcard $(PROJECT_DIR)/src/common/pb/*.cc)
RPCCLIENT_CXX_SRCS = $(wildcard $(PROJECT_DIR)/src/rpcclient/*.cpp)
UTILS_CXX_SRCS = $(wildcard $(PROJECT_DIR)/src/utils/*.cpp)
UTILS_CXX_SRCS +=  $(wildcard $(PROJECT_DIR)/tools/public/*.cpp)
UTILS_C_SRCS = $(wildcard $(PROJECT_DIR)/src/utils/*.c)
CYPRECLIENT_SRCS = $(wildcard $(PROJECT_DIR)/tools/cypreclient/*.cpp)

# Objs
OBJS = $(ACCESS_PB_SRCS:.cc=.o)
OBJS += $(SETMANAGER_PB_SRCS:.cc=.o)
OBJS += $(EXTENTMANAGER_PB_SRCS:.cc=.o)
OBJS += $(EXTENTSERVER_PB_SRCS:.cc=.o)
OBJS += $(COMMON_PB_SRCS:.cc=.o) 
OBJS += $(RPCCLIENT_CXX_SRCS:.cpp=.o)  
OBJS += $(UTILS_CXX_SRCS:.cpp=.o) $(UTILS_C_SRCS:.c=.o)
OBJS += $(CYPRECLIENT_SRCS:.cpp=.o)

# Compile rule
COMPILE_CXX=\
	echo "  CXX $S/$@"; \
	$(CXX) -o $@ $(CXXFLAGS) -c $<

# Link rule
LINK_CXX=\
	echo "  LINK $S/$@"; \
	$(CXX) -o $(PROJECT_DIR)/bin/$@ $(LDFLAGS) $(OBJS) $(BRPC_LIB_LINKER_ARGS) $(DependencyLibs)

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
	rm -f $(PROJECT_DIR)/src/common/pb/*.o
	rm -f $(PROJECT_DIR)/src/extentmanager/pb/*.o
	rm -f $(PROJECT_DIR)/src/extentserver/pb/*.o
	rm -f $(PROJECT_DIR)/src/access/pb/*.o
	rm -f $(PROJECT_DIR)/src/setmanager/pb/*.o	
	rm -f $(PROJECT_DIR)/bin/$(APP)
	rm -f $(PROJECT_DIR)/tools/public/*.o
	rm -f $(PROJECT_DIR)/tools/$(APP)/*.o
