<img alt="pstore logo" src="doc_sources/logo.svg" height="80" width="150">

[![Build Status](https://travis-ci.org/SNSystems/pstore.svg?branch=master)](https://travis-ci.org/SNSystems/pstore)
[![CI Build/Test](https://github.com/SNSystems/pstore/workflows/CI%20Build/Test/badge.svg)](https://github.com/SNSystems/pstore/actions?query=workflow%3A%22CI+Build%2FTest%22)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/15170/badge.svg)](https://scan.coverity.com/projects/snsystems-pstore)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=SNSystems_pstore&metric=alert_status)](https://sonarcloud.io/dashboard?id=SNSystems_pstore)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/SNSystems/pstore.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/SNSystems/pstore/context:cpp)

pstore is a lightweight persistent append-only key/value store intended for use as a back-end for the [LLVM Program Repository](https://github.com/SNSystems/llvm-project-prepo).

Its design goals are:

-   Performance approaching that of an in-memory hash table
-   Good support for parallel compilations
-   Multiple indices
-   In-process

# Table of Contents

-   [Building pstore](#building-pstore)
    -   [Prerequisites](#prerequisites)
    -   [Building](#building)
        -   [Standalone](#standalone)
        -   [Inside LLVM](#inside-llvm)
-   [Getting started](#getting-started)
    -   [Using the read and write utilities](#using-the-read-and-write-utilities)

# Building pstore

## Prerequisites

pstore is built and tested on a variety of platforms:

-   Ubuntu Linux 14.04 LTS Trusty Tahr: building with GCC 5.5.0 and GCC 9.2.1
-   Ubuntu Linux 16.04 LTS Xenial Xerus: building with Clang 3.8.0 and Clang 9.0.1
-   macOS: building with Xcode 9.3
-   Windows: building with Visual Studio 2017 version 15.9

In addition, there’s support for FreeBSD 11, NetBSD 9.1, and Solaris 11.4.

To build it, you’ll also need the following tools:

-   [cmake](http://cmake.org) (version 3.4 or later, version 3.8 or later if using Visual Studio 2017)

Optionally:

-   [Doxygen](http://doxygen.org) for building the documentation
-   [GraphViz](http://graphviz.org) for graph rendering
-   [Node.js](https://nodejs.org/) for the [ElectronJS](https://electronjs.org)-based [broker dashboard](tools/broker_ui/) and some system tests.
-   [Python](https://www.python.org) 2.7 or later for running the system tests as well as a few utilities
-   [Valgrind](http://valgrind.org) for extra validation of the executables (enabled by passing the `-D PSTORE_VALGRIND=Yes` argument when running `cmake` to generate the build)

## Building

pstore may be built either as a standalone collection of libraries, header files, and utilities or as a project within the LLVM framework.

### Standalone

The pstore build system uses cmake. If you’re not very familiar with cmake, there’s a small utility (found in `utils/make_build.py`) which will create an out-of-tree directory in which to build the project and run cmake with the correct arguments for your platform.

~~~bash
python ./utils/make_build.py
cmake ‑‑build build_linux
~~~

The build directory will be one of `build_linux`, `build_mac`, `build_win32`, and so on.

### Inside LLVM

Make sure that pstore is located within the llvm-project directory tree. For example, to build pstore inside LLVM with Program Repository Support ([llvm-project-prepo](https://github.com/SNSystems/llvm-project-prepo)):

~~~bash
git clone http://github.com/SNSystems/llvm-project-prepo.git
cd llvm
git clone http://github.com/SNSystems/pstore.git
cd -
~~~

Build LLVM as [normal](https://llvm.org/docs/CMake.html) enabling the pstore subproject in addition to any others. For example:

~~~~bash
mkdir build
cd build
cmake -G Ninja \
      -D LLVM_ENABLE_PROJECTS="clang;pstore" \
      -D LLVM_TARGETS_TO_BUILD=X86 \
      -D LLVM_TOOL_CLANG_TOOLS_EXTRA_BUILD=Off \
      ../llvm
ninja
~~~~

# Getting started

## Using the read and write utilities

The [pstore-read](tools/read/) and [pstore-write](tools/write/) tools provide a simple way to experiment with the capabilities of the pstore library. Consider the following exchange:

~~~~bash
$ echo foo > foo.txt
$ echo bar > bar.txt
$ pstore-write --add-file mykey,foo.txt pstore.db
$ pstore-read pstore.db mykey
foo
$ pstore-write --add-file mykey,bar.txt pstore.db
$ pstore-read pstore.db mykey
bar
$ pstore-read --revision=1 pstore.db mykey
foo
$ pstore-dump --log pstore.db
---

- file : 
      path : pstore.db
      size : 488
  log  : 
      - { number: 2, size: 56, time: 2020-11-19T15:24:04Z }
      - { number: 1, size: 56, time: 2020-11-19T15:23:52Z }
      - { number: 0, size: 0, time: 2020-11-19T15:23:52Z }
...
$
~~~~

Let’s pick this apart one step at a time…

~~~~bash
$ echo foo > foo.txt
$ echo bar > bar.txt
~~~~

Create two files which contain short text strings that we’ll shortly record in a pstore file:

~~~~bash
$ pstore-write --add-file mykey,foo.txt pstore.db
~~~~

This command creates a new pstore file named `pstore.db` or appends to that file if it already exists. The choice of file name is arbitrary. The tool creates an entry in one of the pstore indexes with the key “mykey” which has a corresponding value string “foo\n” as read from the `foo.txt` file we created earlier.

~~~~bash
$ pstore-read pstore.db mykey
foo
~~~~

Next we use the `pstore-read` utility to search the `pstore.db` file for key named “mykey” and print its value. Happily it prints “foo\n”: the same string that we passed to `pstore-write` above.

~~~~bash
$ pstore-write --add-file mykey,bar.txt pstore.db
~~~~

Now we run `pstore-write` again but this time associating the value “bar” (plus the inevitable newline) with the key “mykey”.

~~~~bash
$ pstore-read pstore.db mykey
bar
~~~~

Running `pstore-read` a second time prints “bar” showing that the key “mykey” has been updated.

~~~~bash
$ pstore-read --revision=1 pstore.db mykey
foo
~~~~

This command is a little more interesting. Here we’ve retrieved the original value that was linked to “mykey”. Each time that `pstore-write` stores data in a pstore file, it does so in a self-contained transaction. Each transaction is appended to the file, preserving the previous contents. The first transaction in a pstore file is number 0 (which is always empty). The first time data is added, transaction 1 is created; the second time, we build transaction 2, and so on. Any redundant data stays in the file &mdash; and is immutable &mdash; until the garbage collector (`pstore-vacuumd`) runs. This property enables a store to be read without any need for locks.

~~~~bash
$ pstore-dump --log pstore.db
~~~~

The [pstore-dump](tools/dump/) command allows us to inspect the innards of a pstore file. It produces a YAML dump of the requested structures: the transaction log in this case showing a list of all of the transactions in the file (newest first), how much data each of them is carrying, and when that data was committed. There is a small number of [other utilities in the tools/ directory](tools/) which allow various aspects of a pstore file to be explored.
