#include "rocksdb-client.h"
#include <stdlib.h>
#include <stdio.h>

int main(void) {
  struct rocksdb_client* db = rocksdb_client_malloc("127.0.0.1", 8000);

  struct handle_or_error r = rocksdb_client_open(db, "./testdb");

  if(!r.valid) abort();

  uint64_t handle = r.handle;

  free_handle_or_error(r);

  const char* k = "hello";
  const char* v = "world";

  struct possible_error e =
    rocksdb_client_put(
      db, handle,
      (const uint8_t*)k, strlen(k) + 1,
      (const uint8_t*)v, strlen(v) + 1);

  if(!e.valid) abort();

  free_possible_error(e);

  struct bytes_or_error b = rocksdb_client_get(db, handle, (const uint8_t*)k, strlen(k) + 1);

  if(!b.valid) abort();

  printf("Value for hello = %s\n", b.bytes.buffer);

  free_bytes_or_error(b);

  rocksdb_client_free(db);

  return 0;
}
