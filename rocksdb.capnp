@0x90e395c23270a303;

interface RocksDB {
  open  @0 (path: Text) -> (handle :UInt64);
  close @1 (handle: UInt64);
  get   @2 (handle: UInt64, key: Data) -> (value: Data);
  put   @3 (handle: UInt64, key: Data, value: Data);
}
