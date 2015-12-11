#ifndef __ROCKSDB_CLIENT_H__
#define __ROCKSDB_CLIENT_H__

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rocksdb_error {
  // TODO(cgaebel): Error codes? At the very least, one for NotFound might be
  // useful.
  char* message;
};

void free_rocksdb_error(struct rocksdb_error);

struct byte_buffer {
  uint8_t* buffer;
  size_t   length;
};

void free_byte_buffer(struct byte_buffer);

/* For any of the below [*_or_error] structs, [error] is the valid union member
   if, and only if, [valid] is false. If [valid] is true, the other union member
   is valid. */

struct handle_or_error {
  bool valid;
  union {
    uint64_t             handle;
    struct rocksdb_error error;
  };
};

void free_handle_or_error(struct handle_or_error);

struct possible_error {
  bool valid;
  union {
    /* Unused, but maintains consistency with the rest of the [*_or_error]
       structs. */
    uint8_t              unused;
    struct rocksdb_error error;
  };
};

void free_posible_error(struct possible_error);

struct bytes_or_error {
  bool valid;
  union {
    struct byte_buffer   bytes;
    struct rocksdb_error error;
  };
};

void free_bytes_or_error(struct bytes_or_error);

struct rocksdb_client;

/* Absolutely do not forget to call [*_free] on the values returned by the
   functions below. Leaking memory is no fun. */

struct rocksdb_client* rocksdb_client_malloc(
  const char* host, uint16_t port);

struct handle_or_error rocksdb_client_open(
  struct rocksdb_client*, const char* database_path);

struct bytes_or_error rocksdb_client_get(
  struct rocksdb_client*, uint64_t handle,
  const uint8_t* key, size_t keylen);

struct possible_error rocksdb_client_put(
  struct rocksdb_client*, uint64_t handle,
  const uint8_t* key, size_t keylen,
  const uint8_t* val, size_t vallen);

void rocksdb_client_free(struct rocksdb_client*);

#ifdef __cplusplus
}
#endif

#endif
