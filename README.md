rocksdb-server
==============

To build:

```
mkdir build/
cmake .. -G Ninja
ninja
```

This creates an executable in your `build/` directory called `rocksdb-server`.
Run it for help text.

To connect to a running server, `#include "rocksdb-client.h"` from C, and link
to `rocksdb-client.a`, `rocksdb-capnp.a` (both in `build/`), your system
capnproto library, and your system libstdc++.

Alternatively, just use cmake and depend on the `rocksdb-client` library.

Currently only lightly tested. Use at your own risk.
