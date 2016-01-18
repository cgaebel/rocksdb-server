cmake_minimum_required(VERSION 2.8)
project(rocksdb-server)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=gnu++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=gnu11")

###############################################################################
## Target Definitions #########################################################
###############################################################################

# rocksdb-server
add_executable(rocksdb-server
  rocksdb-server.cpp)

# rocksdb-client
add_library(rocksdb-client
  rocksdb-client.h
  rocksdb-client.cpp)

add_executable(client-demo
  client-demo.c)

###############################################################################
## Dependencies ###############################################################
###############################################################################

# Capnproto

find_package(CapnProto REQUIRED)
set(CAPNPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CAPNPC_OUTPUT_DIR})
add_definitions(${CAPNP_DEFINITIONS})
capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS rocksdb.capnp)

# rocksdb-capnp
add_library(rocksdb-capnp
  ${CAPNP_SRCS} ${CAPNP_HDRS})
target_link_libraries(rocksdb-capnp ${CAPNP_LIBRARIES})

target_link_libraries(rocksdb-server rocksdb-capnp)
target_link_libraries(rocksdb-client rocksdb-capnp)

# rocksdb

find_package(rocksdb REQUIRED)
include_directories(${ROCKSDB_INCLUDE_DIR})
target_link_libraries(rocksdb-server ${ROCKSDB_LIBRARIES})
target_link_libraries(rocksdb-client ${ROCKSDB_LIBRARIES})

target_link_libraries(client-demo rocksdb-client)

## Dependencies of rocksdb

# pthread

find_package(Threads)

if(THREADS_FOUND)
  if(THREADS_HAVE_PTHREAD_ARG)
    target_compile_options(PUBLIC rocksdb-server "-pthread")
  endif()
  if(CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(rocksdb-server "${CMAKE_THREAD_LIBS_INIT}")
  endif()
endif()

# bz2

find_package (BZip2 REQUIRED)
include_directories(${BZIP_INCLUDE_DIRS})
target_link_libraries (rocksdb-server ${BZIP2_LIBRARIES})

# zlib

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
target_link_libraries(rocksdb-server ${ZLIB_LIBRARIES})

# snappy

find_package(Snappy REQUIRED)
include_directories(${SNAPPY_INCLUDE_DIR})
target_link_libraries(rocksdb-server ${SNAPPY_LIBRARIES})
