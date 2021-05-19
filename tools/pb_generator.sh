#!/bin/bash

SRC_DIR=../

protoc --proto_path=${SRC_DIR}/src --cpp_out=${SRC_DIR}/src ${SRC_DIR}/src/setmanager/pb/set.proto \
                                                            ${SRC_DIR}/src/setmanager/pb/heartbeat.proto \
                                                            ${SRC_DIR}/src/extentmanager/pb/resource.proto \
                                                            ${SRC_DIR}/src/extentmanager/pb/cluster.proto \
                                                            ${SRC_DIR}/src/extentmanager/pb/router.proto \
                                                            ${SRC_DIR}/src/extentmanager/pb/heartbeat.proto \
                                                            ${SRC_DIR}/src/extentserver/pb/extent_io.proto \
                                                            ${SRC_DIR}/src/extentserver/pb/extent_control.proto \
                                                            ${SRC_DIR}/src/access/pb/access.proto \
                                                            ${SRC_DIR}/src/common/pb/types.proto
