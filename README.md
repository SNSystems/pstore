# pstore

pstore is a lightweight persistent append-only key/value store intended for use as a back-end for the LLVM Program Repository ([llvm-prepo](https://github.com/SNSystems/llvm-prepo)).

Its design goals are:

- Performance approaching that of an in-memory hash table
- Good support for parallel compilations
- Multiple indices
- In-process

## Platforms

pstore is built and tested on a variety of platforms:

- Windows (building wth Visual Studio 2017 version 15.4.1)
- macOS (building with Xcode 9.0)
- Linux (Ubuntu 16.04 LTS building with GCC 6.3)

## Prerequisites

- cmake (version 3.1 or later)
- Doxygen or building the documentation
- GraphViz for doxygen diagrams
- Python 2.7 or later for running the system tests

## Building

The build system uses cmake. If you're not very familiar with cmake, there's a small utility (found in `utils/make_build.py`) which will create an out-of-tree directory in which to build the project and run cmake with the correct arguments for your platform.

    $ python ./utils/make_build.py
    $ cmake ‑‑build build_linux

The build directory will be one of `build_linux`, `build_mac`, `build_win32`, and so on.

