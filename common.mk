#
# Copyright 2020 JDD authors.
# @yangchunxin3
#

include $(CYPRESTORE_ROOT_DIR)/env.mk

# compiler
CXX = c++
AR = ar

# -g default debug information, -g0 no debug information, -g1 minimal debug information, -g3 maximal debug information
CXXFLAGS += -g
# sets the compiler's optimization level, O0/1/2/3
CXXFLAGS += -O2
# enables all instruction subsets supported by the local machine 
CXXFLAGS += -march=native
CXXFLAGS += -Wall -Wextra
CXXFLAGS += -Wno-unused-parameter -Wno-missing-field-initializers
CXXFLAGS += -Wformat -Wformat-security
CXXFLAGS += -Wmissing-declarations
CXXFLAGS += -D_GNU_SOURCE -D_FORTIFY_SOURCE=2
CXXFLAGS += -DNDEBUG -DGFLAGS_NS=gflags
CXXFLAGS += -fPIC -fstack-protector -fno-strict-aliasing -fno-common
CXXFLAGS += -std=c++11
CXXFLAGS += -pthread
CXXFLAGS += -I$(CYPRESTORE_ROOT_DIR)/src
CXXFLAGS += -I$(BRPC_HEADER_DIR)
#CXXFLAGS += -DBRPC_ENABLE_CPU_PROFILER

# link flags
LDFLAGS += -Wl,-z,relro,-z,now
LDFLAGS += -Wl,-z,noexecstack

# lib list
LIBS += -pthread -lnuma -lgflags -lleveldb -lprotobuf
LIBS += -lrt -ldl -luuid -lssl -lcrypto -laio
LIBS += $(BRPC_LIB_DIR)/libbrpc.a #-ltcmalloc_and_profiler

# compile rule
COMPILE_CXX=\
	echo "  CXX $S/$@"; \
	$(CXX) -o $@ $(CXXFLAGS) -c $<

# link rule & install command
ifneq (,$(findstring lib, $(APP)))
	LINK_CXX= echo " LINK library $S/$@";\
	$(AR) r $(APP)_static.a $(OBJS); \
	$(CXX) -fPIC -Wall -shared -o $(APP).so $(LDFLAGS) $(OBJS) $(LIBS)

	INSTALL_CMD= mkdir -p $(CYPRESTORE_ROOT_DIR)/lib; \
	mv $(APP)_static.a $(CYPRESTORE_ROOT_DIR)/lib; \
	mv $(APP).so $(CYPRESTORE_ROOT_DIR)/lib
else
	LINK_CXX=\
		echo "  LINK $S/$@"; \
		$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)

	INSTALL_CMD= mkdir -p $(CYPRESTORE_ROOT_DIR)/bin; \
		mv $(APP) $(CYPRESTORE_ROOT_DIR)/bin
endif

.PHONY : clean

all : $(APP)
	@:

$(APP) : $(OBJS)
	$(LINK_CXX)

%.o : %.cpp %.cc
	$(COMPILE_CXX)

install: $(APP)
	echo "do install"
	$(INSTALL_CMD)

