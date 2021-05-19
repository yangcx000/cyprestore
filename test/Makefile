#
# Copyright 2020 JDD authors.
# @yangchunxin3
#

CYPRESTORE_ROOT_DIR = $(abspath ($CURDIR)/../..)

include $(CYPRESTORE_ROOT_DIR)/env.mk

APP = \
	utils_unittest_main \
	common_unittest_main \
	kvstore_unittest_main \
	extentserver_unittest_main \
	extentmanager_unittest_main

# compiler
CXX = c++

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
CXXFLAGS += -fprofile-arcs -ftest-coverage
CXXFLAGS += -I$(CYPRESTORE_ROOT_DIR)/src
CXXFLAGS += -I$(BRPC_HEADER_DIR)
CXXFLAGS += -I$(ROCKSDB_HEADER_DIR)
CXXFLAGS += -I$(SPDK_HEADER_DIR)
CXXFLAGS += -I$(DPDK_HEADER_DIR)

# link flags
LDFLAGS += -Wl,-z,relro,-z,now
LDFLAGS += -Wl,-z,noexecstack
LDFLAGS += -fprofile-arcs -ftest-coverage

# lib list
LIBS += -pthread -lnuma -lgflags -lleveldb -lprotobuf
LIBS += -lrt -ldl -luuid -lssl -lcrypto -laio -lboost_thread -lboost_regex
LIBS += -lsnappy -lz -llz4 -lbz2
LIBS += $(BRPC_LIB_DIR)/libbrpc.a #-ltcmalloc_and_profiler
LIBS += -L${ROCKSDB_LIB_DIR} -lrocksdb
LIBS += -lgtest

# SPDK lib names
SPDK_LIB_NAMES += bdev_nvme bdev_error bdev_aio event_bdev event_accel event_vmd vmd nvme
SPDK_LIB_NAMES += bdev_rpc bdev accel event thread util conf trace log jsonrpc json rpc sock notify
SPDK_LIB_NAMES += env_dpdk  

# DPDK lib names
DPDK_LIB_NAMES = rte_eal rte_mempool rte_ring rte_mbuf rte_mempool_ring rte_pci rte_bus_pci rte_kvargs

LIBS += -L${SPDK_LIB_DIR} -Wl,--whole-archive -Wl,--no-as-needed $(SPDK_LIB_NAMES:%=-lspdk_%) -Wl,--no-whole-archive
LIBS += -L${DPDK_LIB_DIR} -Wl,--whole-archive -Wl,--no-as-needed $(DPDK_LIB_NAMES:%=-l%) -Wl,--no-whole-archive
LIBS += -lsnappy -lz -llz4 -lbz2

# utils
UTILS_SOURCES = \
	$(CYPRESTORE_ROOT_DIR)/src/utils/ini_parser.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/timer_thread.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/coding.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/crc32.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/chrono.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/kvstore/rocks_store.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/regex_util.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/gcd_util.cpp

UTILS_UNITTEST_SOURCES = \
	ini_parser_unittest.cpp \
	timer_thread_unittest.cpp \
	serializer_unittest.cpp \
	regex_unittest.cpp \
	gcd_unittest.cpp \
	crc32_unittest.cpp \
	utils_unittest_main.cpp

UTILS_OBJS = $(addsuffix .o, $(basename $(UTILS_SOURCES))) 
UTILS_UNITTEST_OBJS = $(addsuffix .o, $(basename $(UTILS_UNITTEST_SOURCES))) 

# common
COMMON_SOURCES = \
	$(CYPRESTORE_ROOT_DIR)/src/common/config.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/common/arena.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/common/cypre_ring.cpp \
    $(CYPRESTORE_ROOT_DIR)/src/extentserver/io_mem.cpp \
    $(CYPRESTORE_ROOT_DIR)/src/extentserver/nvme_device.cpp \
    $(CYPRESTORE_ROOT_DIR)/src/extentserver/spdk_worker.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/spdk_mgr.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/spdk_worker.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/request_context.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/io_mem.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/ini_parser.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/crc32.cpp

COMMON_UNITTEST_SOURCES = \
	config_unittest.cpp \
	rwlock_unittest.cpp \
    nvme_device_unittest.cpp \
	arena_unittest.cpp \
	mem_buffer_unittest.cpp \
	ctxmem_mgr_unittest.cpp \
	common_unittest_main.cpp

COMMON_OBJS = $(addsuffix .o, $(basename $(COMMON_SOURCES))) 
COMMON_UNITTEST_OBJS = $(addsuffix .o, $(basename $(COMMON_UNITTEST_SOURCES))) 

# kvstore
KVSTORE_SOURCES = \
	$(CYPRESTORE_ROOT_DIR)/src/kvstore/rocks_store.cpp

KVSTORE_UNITTEST_SOURCES = \
	kvstore_unittest_main.cpp \
	rocks_store_unittest.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/uuid.cpp

KVSTORE_OBJS = $(addsuffix .o, $(basename $(KVSTORE_SOURCES)))
KVSTORE_UNITTEST_OBJS = $(addsuffix .o, $(basename $(KVSTORE_UNITTEST_SOURCES)))

# extentserver
EXTENTSERVER_SOURCES = \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/bitmap_allocator.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/space_alloc.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/block_device.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/nvme_device.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/spdk_worker.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/spdk_mgr.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/storage_engine.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/bare_engine.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/replicate_engine.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/request_context.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/io_mem.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/extent_location.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/extentserver.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/heartbeat_reporter.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/extent_io_service.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/extent_control_service.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/pb/extent_io.pb.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentserver/pb/extent_control.pb.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/common/config.cpp \
    $(CYPRESTORE_ROOT_DIR)/src/common/constants.cpp \
    $(CYPRESTORE_ROOT_DIR)/src/common/log.cpp \
    $(CYPRESTORE_ROOT_DIR)/src/common/extent_router.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/common/arena.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/common/cypre_ring.cpp \
    $(CYPRESTORE_ROOT_DIR)/src/utils/chrono.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/ini_parser.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/timer_thread.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/tools.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/uuid.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/coding.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/crc32.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/kvstore/rocks_store.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/common/pb/types.pb.cp \
	$(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/heartbeat.pb.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentmanager/pb/router.pb.cpp

EXTENTSERVER_UNITTEST_SOURCES = \
	extentserver_unittest_main.cpp \
	bitmap_allocator_unittest.cpp \
	space_alloc_unittest.cpp \
	iomem_unittest.cpp \
	iomem_mgr_unittest.cpp \
	extent_location_unittest.cpp

EXTENTSERVER_OBJS = $(addsuffix .o, $(basename $(EXTENTSERVER_SOURCES)))
EXTENTSERVER_UNITTEST_OBJS = $(addsuffix .o, $(basename $(EXTENTSERVER_UNITTEST_SOURCES)))

# extentmanager
EM_SOURCES = \
	$(CYPRESTORE_ROOT_DIR)/src/extentmanager/rg_allocater_by_es.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentmanager/rg_allocater_by_host.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/extentmanager/rg_allocater_by_rack.cpp \
	$(CYPRESTORE_ROOT_DIR)/src/utils/chrono.h \
	$(CYPRESTORE_ROOT_DIR)/src/utils/gcd_util.h

EM_UNITTEST_SOURCES = \
	extentmanager_unittest_main.cpp \
	rg_allocater_unittest.cpp \
	router_unittest.cpp

EM_OBJS = $(addsuffix .o, $(basename $(EM_SOURCES)))
EM_UNITTEST_OBJS = $(addsuffix .o, $(basename $(EM_UNITTEST_SOURCES)))

.PHONY : clean

all : $(APP)
	@:

utils_unittest_main : $(UTILS_OBJS) $(UTILS_UNITTEST_OBJS)
	$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)

common_unittest_main : $(COMMON_OBJS) $(COMMON_UNITTEST_OBJS)
	$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)

kvstore_unittest_main : $(KVSTORE_OBJS) $(KVSTORE_UNITTEST_OBJS)
	$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)

extentserver_unittest_main : $(EXTENTSERVER_OBJS) $(EXTENTSERVER_UNITTEST_OBJS)
	$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)

extentmanager_unittest_main : $(EM_OBJS) $(EM_UNITTEST_OBJS)
	$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)


%.o : %.cpp %.cc
	$(CXX) -o $@ $(CXXFLAGS) -c $<

clean :
	rm -f $(CYPRESTORE_ROOT_DIR)/src/common/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/common/*.gcno
	rm -f $(CYPRESTORE_ROOT_DIR)/src/common/*.gcda
	rm -f $(CYPRESTORE_ROOT_DIR)/src/utils/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/utils/*.gcno
	rm -f $(CYPRESTORE_ROOT_DIR)/src/utils/*.gcda
	rm -f $(CYPRESTORE_ROOT_DIR)/src/kvstore/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/kvstore/*.gcno
	rm -f $(CYPRESTORE_ROOT_DIR)/src/kvstore/*.gcda
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentserver/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentserver/*.gcno
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentserver/*.gcda
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentmanager/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentmanager/*.gcno
	rm -f $(CYPRESTORE_ROOT_DIR)/src/extentmanager/*.gcda
	rm -f $(CYPRESTORE_ROOT_DIR)/test/*.o
	rm -f $(CYPRESTORE_ROOT_DIR)/test/*.gcno
	rm -f $(CYPRESTORE_ROOT_DIR)/test/*.gcda
	rm -f $(CYPRESTORE_ROOT_DIR)/test/*.info
	rm -rf $(CYPRESTORE_ROOT_DIR)/test/report
	rm -f $(CYPRESTORE_ROOT_DIR)/test/common_unittest_main
	rm -f $(CYPRESTORE_ROOT_DIR)/test/utils_unittest_main
	rm -f $(CYPRESTORE_ROOT_DIR)/test/kvstore_unittest_main
	rm -f $(CYPRESTORE_ROOT_DIR)/test/extentserver_unittest_main
	rm -f $(CYPRESTORE_ROOT_DIR)/test/extentmanager_unittest_main
