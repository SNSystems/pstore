<img alt="pstore logo" src="doc_sources/logo.svg" height="" height="80" width="150" />

[![Linux and macOS Build Status](https://travis-ci.org/SNSystems/pstore.svg?branch=master)](https://travis-ci.org/SNSystems/pstore)
[![Windows Build status](https://ci.appveyor.com/api/projects/status/ckl6dh2i3eb2u33e?svg=true)](https://ci.appveyor.com/project/paulhuggett/pstore)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/15170/badge.svg)](https://scan.coverity.com/projects/snsystems-pstore)

pstore is a lightweight persistent append-only key/value store intended for use as a back-end for the [LLVM Program Repository](https://github.com/SNSystems/llvm-prepo).

Its design goals are:

- Performance approaching that of an in-memory hash table
- Good support for parallel compilations
- Multiple indices
- In-process

# Building pstore

## Prerequisites

pstore is built and tested on a variety of platforms:

- Linux (Ubuntu 16.04 LTS building with GCC 6.3)
- macOS (building with Xcode 9.0)
- Windows (building wth Visual Studio 2017 version 15.8)

(In addition, there’s basic support for FreeBSD 11 and Solaris 11.4  beta using clang 4.0.0 and GCC 5.5.0 respectively.)

To build it, you’ll also need the following tools:

- [cmake](http://cmake.org) (version 3.6 or later)

Optionally:

- [Doxygen](http://doxygen.org) for building the documentation
- [GraphViz](http://graphviz.org) for graph rendering
- [Python](https://www.python.org) 2.7 or later for running the system tests as well as a few utilities
- [Valgrind](http://valgrind.org) for extra validation of the executables (enabled by passing the `-D PSTORE_VALGRIND=Yes` argument when running `cmake` to generate the build)

## Building

The pstore build system uses cmake. If you’re not very familiar with cmake, there’s a small utility (found in `utils/make_build.py`) which will create an out-of-tree directory in which to build the project and run cmake with the correct arguments for your platform.

    $ python ./utils/make_build.py
    $ cmake ‑‑build build_linux

The build directory will be one of `build_linux`, `build_mac`, `build_win32`, and so on.

# Getting started
## Using the read and write utilities

The [pstore-read](tools/read/) and [pstore-write](tools/write/) tools provide a simple way to experiment with the capabilities of the pstore library. Consider the following exchange:

    $ echo foo > foo.txt
    $ echo bar > bar.txt
    $ pstore-write --add-file mykey,foo.txt pstore.db
    $ pstore-read pstore.db mykey
    foo
    $ pstore-write --add-file mykey,bar.txt pstore.db
    $ pstore-read pstore.db mykey
    bar
    $ pstore-read -r 1 pstore.db mykey
    foo
    $ pstore-dump --log pstore.db
    ---

    - file : 
          path : pstore.db
          size : 4194304
      log  : 
          - { number: 2, size: 32, time: 2018-03-12T13:52:04Z }
          - { number: 1, size: 32, time: 2018-03-12T13:51:38Z }
          - { number: 0, size: 0, time: 2018-03-12T13:51:38Z }
    ...
    $ 

Let’s pick this apart one step at a time…

    $ echo foo > foo.txt
    $ echo bar > bar.txt

Create two files which contain short text strings that we’ll shortly record in a pstore file:

    $ pstore-write --add-file mykey,foo.txt pstore.db

This command creates a new pstore file named `pstore.db` or appends to that file if it already exists. The choice of file name is arbitrary. The tool creates an entry in one of the pstore indexes with the key “mykey” which has a corresponding value string “foo\n” as read from the `foo.txt` file we created earlier.

    $ pstore-read pstore.db mykey
    foo

Next we use the `pstore-read` utility to search the `pstore.db` file for key named “mykey” and print its value. Happily it prints “foo\n”: the same string that we passed to `pstore-write` above.

    $ pstore-write --add-file mykey,bar.txt pstore.db

Now we run `pstore-write` again but this time associating the value “bar” (plus the inevitable newline) with the key “mykey”.

    $ pstore-read pstore.db mykey
    bar

Running `pstore-read` a second time prints “bar” showing that the key “mykey” has been updated.

    $ pstore-read -r 1 pstore.db mykey
    foo

This command is a little more interesting. Here we’ve retrieved the original value that was linked to “mykey”. Each time that `pstore-write` stores data in a pstore file, it does so in a self-contained transaction. Each transaction is appended to the file, preserving the previous contents. The first transaction in a pstore file is number 0 (which is always empty). The first time data is added, transaction 1 is created; the second time, we build transaction 2, and so on. Any redundant data stays in the file &mdash; and is immutable &mdash; until the garbage collector (`pstore-vacuumd`) runs. This property enables a store to be read without any need for locks.

    $ pstore-dump --log pstore.db

The [pstore-dump](tools/dump/) command allows us to inspect the innards of a pstore file. It produces a YAML dump of the requested structures: the transaction log in this case showing a list of all of the transactions in the file (newest first), how much data each of them is carrying, and when that data was committed.

