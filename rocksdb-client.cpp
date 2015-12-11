#include "rocksdb-client.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <cstring>
#include <utility>

#include <kj/debug.h>
#include <capnp/ez-rpc.h>
#include <capnp/message.h>

#include "rocksdb.capnp.h"

extern "C" {

void free_rocksdb_error(struct rocksdb_error e) { free(e.message); }
void free_byte_buffer(struct byte_buffer b) { free(b.buffer); }
void free_handle_or_error(struct handle_or_error h) { if(!h.valid) free_rocksdb_error(h.error); }
void free_posible_error(struct possible_error e) { if(!e.valid) free_rocksdb_error(e.error); }
void free_bytes_or_error(struct bytes_or_error b) { if(b.valid) free_byte_buffer(b.bytes); else free_rocksdb_error(b.error); }

static struct rocksdb_error error_of_exception(kj::Exception& e) {
  struct rocksdb_error ret;
  ret.message = strdup(e.getDescription().cStr());
  return ret;
}

static uint8_t* bdup(const uint8_t* to_copy, size_t len) {
  uint8_t* p = (uint8_t*)malloc(len);
  memcpy(p, to_copy, len);
  return p;
}

static struct rocksdb_error wtf_error() {
  assert(false);
  struct rocksdb_error ret;
  ret.message = strdup("Unknown, uncaught exception. Also, you have asserts disabled.");
  return ret;
}

struct rocksdb_client {
  capnp::EzRpcClient client;
  RocksDB::Client    capability;

  rocksdb_client(const char* host, uint16_t port)
    : client(host, port)
    , capability(client.getMain<RocksDB>())
  {}
};

struct rocksdb_client* rocksdb_client_malloc(
    const char* host, uint16_t port) {
  return new rocksdb_client(host, port);
}

/* Absolutely do not forget to call [*_free] on the values returned by the
   functions below. Leaking memory is no fun. */

struct handle_or_error rocksdb_client_open(
    rocksdb_client* c, const char* path) {
  try {
    auto& wait_scope = c->client.getWaitScope();
    auto request = c->capability.openRequest();

    request.setPath(path);

    auto result = request.send().wait(wait_scope);

    struct handle_or_error handle;
    handle.valid = true;
    handle.handle = result.getHandle();
    return handle;
  } catch(kj::Exception& e) {
    struct handle_or_error error;
    error.valid = false;
    error.error = error_of_exception(e);
    return error;
  } catch(...) {
    struct handle_or_error error;
    error.valid = false;
    error.error = wtf_error();
    return error;
  }
}

struct bytes_or_error rocksdb_client_get(
    struct rocksdb_client* c, uint64_t handle,
    const uint8_t* key, size_t keylen) {
  try {
    auto& wait_scope = c->client.getWaitScope();
    auto request = c->capability.getRequest();

    request.setHandle(handle);
    request.setKey(kj::ArrayPtr<const uint8_t>(key, keylen));

    auto result = request.send().wait(wait_scope);
    auto value = result.getValue();

    struct bytes_or_error bytes;
    bytes.valid = true;
    size_t len = value.size();
    bytes.bytes.buffer = bdup(value.begin(), len);
    bytes.bytes.length = len;
    return bytes;
  } catch(kj::Exception& e) {
    struct bytes_or_error error;
    error.valid = false;
    error.error = error_of_exception(e);
    return error;
  } catch(...) {
    struct handle_or_error error;
    error.valid = false;
    error.error = wtf_error();
    return error;
  }
}

struct possible_error rocksdb_client_put(
    struct rocksdb_client* c, uint64_t handle,
    const uint8_t* key, size_t keylen,
    const uint8_t* val, size_t vallen) {
  try {
    auto& wait_scope = c->client.getWaitScope();
    auto request = c->capability.putRequest();

    request.setHandle(handle);
    request.setKey(kj::ArrayPtr<const uint8_t>(key, keylen));
    request.setValue(kj::ArrayPtr<const uint8_t>(val, vallen));

    auto result = request.send().wait(wait_scope);

    struct possible_error no_error;
    no_error.valid  = true;
    return no_error;
  } catch(kj::Exception& e) {
    struct possible_error error;
    error.valid = false;
    error.error = error_of_exception(e);
    return error;
  } catch(...) {
    struct possible_error error;
    error.valid = false;
    error.error = wtf_error();
    return error;
  }
}

void rocksdb_client_free(struct rocksdb_client* c) {
  delete c;
}

}
