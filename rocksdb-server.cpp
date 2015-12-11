#include <stdint.h>
#include <time.h>

#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include <kj/async-io.h>
#include <kj/debug.h>
#include <kj/time.h>
#include <capnp/ez-rpc.h>
#include <capnp/message.h>

#include <rocksdb/db.h>

#include "rocksdb.capnp.h"

struct DB {
  std::unique_ptr<rocksdb::DB> db;
  uint64_t                     refcount;

  DB() : db(), refcount(1) {}

  rocksdb::Status init(const std::string& file) {
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::DB* ptr = NULL;
    rocksdb::Status status = rocksdb::DB::Open(options, file, &ptr);
    db = std::unique_ptr<rocksdb::DB>(ptr);
    return status;
  }
};

struct RocksdbServer : public RocksDB::Server {
  std::unordered_map<std::string, uint64_t> handle_map;
  std::unordered_map<uint64_t, std::string> rev_handle_map;
  std::unordered_map<uint64_t, DB>          connections;
  uint64_t                                  next_handle_to_allocate;

  RocksdbServer()
    : connections(), next_handle_to_allocate(0)
  {}

  void invariant() {
    for(auto& x : handle_map) {
      KJ_REQUIRE(rev_handle_map.find(x.second) != rev_handle_map.end(),
                 "rev_handle_map must be a subset of handle_map");
      KJ_REQUIRE(connections.find(x.second) != connections.end(),
                 "handles in the handle_map must have connections");
    }
    for(auto& x : rev_handle_map) {
      KJ_REQUIRE(handle_map.find(x.second) != handle_map.end(),
                 "handle_map must be a subset of rev_handle_map");
    }
    for(auto& x : connections) {
      KJ_REQUIRE(x.first < next_handle_to_allocate,
                 "there can be no reallocation of handles");
      KJ_REQUIRE(rev_handle_map.find(x.first) != rev_handle_map.end(),
                 "connections must all be in handle_map");
    }
  }

  kj::Promise<void> open(OpenContext context) {
    invariant();

    auto filename = context.getParams().getPath();
    auto id_handle = handle_map.find(filename);

    if(id_handle == handle_map.end()) {
      // Not yet connected to.
      uint64_t id = next_handle_to_allocate++;
      DB db;

      rocksdb::Status init_result = db.init(filename);

      // Ensure connected.
      KJ_REQUIRE(init_result.ok(), "Could not connect to database.", init_result.ToString());

      handle_map.emplace(filename, id);
      rev_handle_map.emplace(id, filename);
      connections.emplace(id, std::move(db));
      context.getResults().setHandle(id);
    } else {
      // Already connected.
      uint64_t id = id_handle->second;
      DB& connection = connections[id];
      connection.refcount++;
      context.getResults().setHandle(id);
    }

    invariant();

    return kj::READY_NOW;
  }

  void unmap_except_rev_map(uint64_t id, std::vector<uint64_t>* ids_to_remove_later) {
    if(ids_to_remove_later != nullptr)
      ids_to_remove_later->push_back(id);

    handle_map.erase(handle_map.find(rev_handle_map.find(id)->second));
    connections.erase(connections.find(id));
  }

  void unmap(uint64_t id) {
    unmap_except_rev_map(id, nullptr);
    rev_handle_map.erase(rev_handle_map.find(id));
  }

  kj::Promise<void> close(CloseContext context) {
    invariant();

    uint64_t id = context.getParams().getHandle();

    DB& connection = connections[id];
    connection.refcount--;

    if(connection.refcount == 0)
      unmap(id);

    invariant();

    return kj::READY_NOW;
  }

  DB* db_of(uint64_t id) {
    auto iter = connections.find(id);
    KJ_REQUIRE(iter != connections.end(), "Invalid connection id.", id);
    return &iter->second;
  }

  static rocksdb::Slice slice_of_kj(kj::ArrayPtr<const uint8_t> kj) {
    return rocksdb::Slice(
      (const char*)kj.begin(),
      kj.size());
  }

  static kj::ArrayPtr<const uint8_t> kj_of_string(const std::string& s) {
    return kj::ArrayPtr<const uint8_t>((const uint8_t*)s.c_str(), s.size());
  }

  kj::Promise<void> get(GetContext context) {
    auto params = context.getParams();
    uint64_t id = params.getHandle();
    auto key    = slice_of_kj(params.getKey());

    DB* db = db_of(id);

    std::string value;

    rocksdb::Status s = db->db->Get(rocksdb::ReadOptions(), key, &value);
    KJ_REQUIRE(s.ok(), "RocksDB Error", s.ToString());

    context.getResults().setValue(kj_of_string(value));

    return kj::READY_NOW;
  }

  kj::Promise<void> put(PutContext context) {
    auto params = context.getParams();
    uint64_t id = params.getHandle();
    auto key    = slice_of_kj(params.getKey());
    auto value  = slice_of_kj(params.getValue());

    DB* db = db_of(id);

    rocksdb::Status s = db->db->Put(rocksdb::WriteOptions(), key, value);
    KJ_REQUIRE(s.ok(), "RocksDB Error", s.ToString());

    return kj::READY_NOW;
  }

  virtual ~RocksdbServer() {}
};

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr
      << "usage: " << argv[0] << " ADDRESS[:PORT]\n"
        "Runs the server bound to the given address/port.\n"
        "ADDRESS may be '*' to bind to all local addresses.\n"
        ":PORT may be omitted to choose a port automatically.\n"
        "\n"
        "The address format \"unix:/path/to/socket\" opens a unix\n"
        "domain socket."
      << std::endl;
    return 1;
  }

  capnp::EzRpcServer server(kj::heap<RocksdbServer>(), argv[1]);

  auto& wait_scope = server.getWaitScope();

  uint port = server.getPort().wait(wait_scope);
  if(port == 0)
    std::cout << "Listening on unix socket [" << argv[1] << "]..." << std::endl;
  else
    std::cout << "Listening on port " << port << "..." << std::endl;

  kj::NEVER_DONE.wait(wait_scope);
}
