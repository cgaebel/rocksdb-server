cmake_minimum_required(VERSION 2.8.11)
project(rocksdb-server)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(CapnProto REQUIRED)
find_package(rocksdb REQUIRED)

set(CAPNPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CAPNPC_OUTPUT_DIR})

include_directories(${CAPNP_INCLUDE_DIRS} ${ROCKSDB_INCLUDE_DIR})
add_definitions(${CAPNP_DEFINITIONS})

capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS rocksdb.capnp)
add_executable(rocksdb-server rocksdb-server.cpp ${CAPNP_SRCS} ${CAPNP_HDRS})
target_link_libraries(rocksdb-server ${CAPNP_LIBRARIES} ${ROCKSDB_LIBRARIES})

# needed for capnp
set_property(TARGET rocksdb-server PROPERTY CXX_STANDARD 11)
