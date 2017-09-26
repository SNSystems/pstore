pstore
==
pstore is a lightweight persistent append-only key/value store intended for use as a back-end for the LLVM Program Repository (llvm-prepo).

Its design goals are:

- Performance approaching that of an in-memory hash table
- Good support for parallel compilations
- Multiple indices
- In-process
