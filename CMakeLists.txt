cmake_minimum_required(VERSION 2.8.11)
project(rocksdb-server)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# Capnproto

find_package(CapnProto REQUIRED)
set(CAPNPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CAPNPC_OUTPUT_DIR})
add_definitions(${CAPNP_DEFINITIONS})
capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS rocksdb.capnp)

# RocksDB

find_package(rocksdb REQUIRED)
include_directories(${ROCKSDB_INCLUDE_DIR})

# rocksdb-capnp

add_library(rocksdb-capnp
  ${CAPNP_SRCS} ${CAPNP_HDRS})
target_link_libraries(rocksdb-capnp ${CAPNP_LIBRARIES})

# rocksdb-server

add_executable(rocksdb-server
  rocksdb-server.cpp)

target_link_libraries(rocksdb-server
  rocksdb-capnp
  ${ROCKSDB_LIBRARIES})

# rocksdb-client

add_library(rocksdb-client
  rocksdb-client.cpp)

target_link_libraries(rocksdb-client
  rocksdb-capnp)

# needed for capnp
set_property(TARGET rocksdb-capnp  PROPERTY CXX_STANDARD 11)
set_property(TARGET rocksdb-server PROPERTY CXX_STANDARD 11)
set_property(TARGET rocksdb-client PROPERTY CXX_STANDARD 11)
