/**

\page data_store_structure Data store structure

# PStore: A program graph repository #

## Introduction

Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.

A fast, efficient, data store for the program repository.

You'll notice that I usually refer to PStore as a "data store" rather than a "database". This is intended to convey that the library has a relatively simple, almost, crude interface with none of the niceties (such as a query language) that one would expect from a "database".

## Design requirements

1. Efficiency.
2. Supports fast concurrent access.
3. Server-less. Avoid a single central process which runs the danger of introducing configuration complexity and becoming a performance bottleneck.
4. Crash resistant. A host process crashing should not result in corruption of the data store.
5. Friendly to distributed build systems

### Server-less

Interaction with distributed build systems (e.g. SN-DBS, distcc, IncrediBuild) is important for the maximum compilation speed. These tools distribute the build over a number of separate machines. This means that although the program repository is trying to build a complete image of a program, the distribution makes this tricky. One approach might be to run a single centralized server into which all of the remote slaves write their contributions to the overall program. However, this creates a number of administrative issues:

> Weak. Weak.

- Are corporate firewalls suitably configured to allow the network traffic to flow between the machines?
- How do we ensure that the database server is running when necessary without requiring complex installation to setup daemon processes?

### Security

On the basis that traditional object files offer no particular properties to secure their contents, it is assumed for the time being that no security features are required of the data store. If it turns out that this could be a desirable

### CRUD

CRUD operations — create, read, update, and delete — are the basis of any effective data store. It must be possible to store data, to locate and retrieve it, update it, and remove it. PStore must meet these requirements.

### ACID

The usual database ACID (Atomicity, Consistency, Isolation, Durability) requirements are largely also required for a program repository, but only to the extent that they are satisfied by conventional object files.

__Atomicity__: We need to ensure that data store transactions can be updated incrementally as, for example, each function's code is produced by the compiler. Although not a guarantee provided by a traditional object files, it would be useful if it were possible to "undo" those incremental modifications if the compiler is killed.

__Consistency__: The consistency property ensures that any transaction will bring the database from one valid state to another. The only state being modified at each transaction should be the transaction machinery itself, and the validity of the indices that are used by clients to access the correct versions of the stored data.

__Isolation__: We must ensure that two threads attempting to the data store do so without interacting with one-another.

__Durability__: The durability requirements on the program data store is relatively limited by the standards of commercial database systems. This relaxation is to ensure that best throughput for the system. Once a transaction is committed, it need only rely on the operating system to flush this data back to disk as necessary. If the operating system crashes, then the integrity of the data is not guaranteed. This level of warranty is similar to that of the object files that are used by a traditional compilation system.

> TODO: investigate win32 FlushViewOfFile() and FlushFileBuffers(). What would be the performance impact, if any, if this is called when a transaction is committed? This (and the POSIX equivalent) would give full ACID-compliant durability.

# Append-Only

The PStore design uses the append-only model although the use of memory-mapped files for data access means that it cannot be a true append-only implementation. This approach makes the data from a committed transaction immutable and therefore enables the use of non-blocking algorithms for access to the stored data in multiple threads and processes.

The only lock necessary is to ensure that a single thread appends to the store at time. Although it does not block readers, this lock acts on the _entire_ data store so it important that a client application holds it for the shortest possible time.


## Garbage Collection

Naturally, one of the trade-offs with an append-only design is that old versions of data accumulate over time, bloating the data store. For this reason, we need to use a garbage collection process to dispose of old, unneeded, data. The periodicity of this garbage collection depends on:

- The availability of storage. If storage is both fast and plentiful, then we can afford to perform relatively infrequent garbage collection. The cost of the storage is lower than the cost of running the process to recover storage.
- The rate of growth. If the data store is growing rapidly -- a large volume of change being regularly recorded -- then garbage collection must be performed more frequently.

A heuristic is employed to determine when the garbage collector is used, although explicit, manual, control is also possible.


## Multi-version Concurrency Control (MVCC)

By treating old versions of data as immutable, we gain the useful property that a consumer can retain a coherent and consistent view of the state of the data store regardless of any changes that are added to it. This obviates the need for synchronization mechanisms, such as mutexes, to ensure that consistency; this is, in turn, good for the performance of client applications.


# Implementation

This page describes the various layers of the core data store design. This is the base layer onto which higher level abstractions such as concurrency management, transactions and indices are built.

The design has been focussed on using memory mapped files to provide the highest performance on the supported operating systems. This starting pointer has led to a design which, implemented as a series of tiers, which allow for the data store to grow as necessary whilst providing a simple, consistent, interface for data access and storage.

The basic design is shown in the diagram below:

\image html memory_mapping.svg

This shows, starting at the bottom, that we have 4 core concepts:

1. \ref physical_file
2. \ref memory_mapping_views
3. \ref segment_address_table
4. \ref address_structure

\section physical_file Physical File

At the lowest level, we have the physical file on the user's disk. Like any file, this can be viewed as a variable-length contiguous stream of bytes. Unfortunately, the fact that the data store must be of variable length is not immediately compatible with our desire to memory-map the file.

On Windows platforms, each memory-mapped byte must correspond to a physical byte in the file. The size of the memory-mapped region is determined when the memory mapped "view" (to use Microsoft's terminology) is created: if the file is not large enough at that point then it is automatically grown by the operating system. The operating system does not support sparse files, so this is an expensive operation.

\section memory_mapping_views Memory Mapped Views

There are a few mutually contradictory requirements with which the design must wrestle:

- We'd like to create as few memory mapping views as possible to avoid using too much of the system's resources
- We need to keep the size of the memory mapping views to something reasonable to avoid requesting too much contiguous address space: the larger the request, the greater the chance that it will be rejected.
- We need to ensure that views added to accommodate growth in the file are small to ensure that we avoid growing the physical file unnecessarily (for Windows).

When the file is opened, we map 0 or more "full size regions" of 2<sup>32</sup> bytes each. Any remaining bytes are mapped as a single region whose size is a multiple of 2<sup>12</sup> bytes.


All of these views are recorded in the "map view table" vector. As new views are added when the data store grows, additional values are added to the end of this container.

Note that Windows property of requiring that there are physical bytes in the file for each byte that is memory-mapped prevents us from implementing a true "append-only" design. In a conventional design, the end of th


## File Compaction/Garbage Collection

> How does it work?

\section segment_address_table Segment Address Table

We divide up the memory-mapped blocks into a series of equal-sized "segments". Each segment corresponds to an entry in the "segment address table". This is a fixed-size vector of pstore::database::sat_elements entries. Each of the entries points to a 2<sup>12</sup> (4Mb) block contained within an instance of pstore::region::memory_mapper_base.

\section address_structure "address" Structure

The "address" structure is the data store's equivalent of a "pointer". Using an address, it's possible to uniquely access any data (and meta-data) through the memory-mapped views. Addresses are machine independent, meaning that they remain valid regardless of the host operating system and are not affected by the physical address at which data is mapped.

An "address" consists of two parts:

- A _segment number_. The segment number is used as an index into the SAT: this lookup yields the physical address of the 4Mb block.
- A _segment offset_. The segment offset provides the offset within this 4Mb region.



# Transactions

Transactions are the backbone of the ACID guarantees and of the PStore synchronization mechanism. Before any data may be written to the store, an instance of pstore::transaction is created. This will block, using a global named mutex (pstore::named_mutex) whose name is given by pstore::header::sync_name, until no other thread has an active transaction.

When a transaction is completed, the pstore::header::footer_pos is updated; until this happens the newly update data is invisible to other consumers. A transaction is aborted by simply doing nothing; any new data beyond the end of the last transaction is overwritten by the next transaction.

The initial state of a PStore data file is shown below. The file simply contains its header structure an an initial (empty) transaction (_t_<sub>0</sub>).

\image html store_file_format_t0.svg

The header and footer types are pstore::header and pstore::trailer respectively. The state of the file after the first transaction (_t_<sub>1</sub>) has been committed:

\image html store_file_format_t1.svg

The data block shown as data(_t_<sub>1</sub>) includes the real data payload as well as index meta-data.

> The structure of this data block will be discussed later.

A thread connecting to the data store uses the pstore::header::footer_pos value to find the most recent completed transaction; this is an instance of pstore::trailer and marks the _end_ of the data associated with that transaction.

# Indices

> Talk about how the indices work.....

Indices are implemented as Hash Array Mapped Tries. Forming a rough hybrid between the hash tables often used for fast in-memory lookup and the B-trees used in traditional database systems, it achieves speed that can exceed the former whilst offering some of the advantages of the latter. Usefully whereas a hash table may have to be periodically resized, a HAMT grows dynamically, which is both speed and space efficient. See [Ideal Hash Trees (Phil Bagwell (2000)](http://infoscience.epfl.ch/record/64398/files/idealhashtrees.pdf).

> A sub-page on the inner workings and timings of HAMT?

## Multiple Indexes

- Content
- Name






*/
