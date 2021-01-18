# pstore Tools

## Table of Contents

*   [Installed tools](#installed-tools)
    *   [Exchange](#exchange)
    *   [Exploring](#exploring)
    *   [Garbage Collection](#garbage-collection)
    *   [Read and Write](#read-and-write)
*   [Build\-and\-test tools](#build-and-test-tools)
    *   [Build\-time](#build-time)
    *   [Testing](#testing)

## Installed tools

### Exchange

Tools for textual export and import of the contents of a pstore file.

| Name | Description |
| --- | --- |
| [pstore&#8209;export](export/) | Exports the contents of a pstore repository in a format that can be consumed by pstore&#8209;import. |
| [pstore&#8209;import](import/) | Creates a pstore repository from an export file created by pstore&#8209;export. |

### Exploring

The utility programs in this group enable you to inspect and explore the contents of a pstore database.

| Name | Description |
| --- | --- |
| [pstore&#8209;diff](diff/) | Dumps diff between two pstore revisions. |
| [pstore&#8209;dump](dump/) | Dumps pstore contents as YAML. |
| [pstore&#8209;index&#8209;stats](index_stats/) | Dumps statistics about the index trees in a pstore file as CSV. |
| [pstore&#8209;index&#8209;structure](index_structure/) | Dumps pstore index structures as [GraphViz DOT](https://graphviz.org) graphs. |

### Garbage Collection

| Name | Description |
| --- | --- |
| [pstore&#8209;brokerd](broker/) | The pstore message broker. The broker is a service which is used to manage the garbage collection processes. |
| [pstore&#8209;ui](broker_ui)    | The pstore broker dashboard application. |
| [pstore&#8209;vacuumd](vacuum/) | Data store garbage collector. |


### Read and Write

These utilities are included purely to provide a simple means of experimenting with the pstore library. `pstore-write` enables you to store specific key/value pairs or whole files in a database (where the key is the file path and the value is the file’s contents). `pstore-read`, on the other hand, allows the data that is recorded by `pstore-write` to be extracted by printing it to stdout.

| Name | Description |
| --- | --- |
| [pstore&#8209;read](read/) | A utility for reading the write or strings index. |
| [pstore&#8209;write](write/) | A utility for writing to the write or strings index. |

## Build-and-test tools

### Build-time

Executables that are used as part of the pstore build itself.

| Name | Description |
| --- | --- |
| [pstore&#8209;genromfs](genromfs/) | Dumps a directory structure to stdout in a form that can be used by the [romfs](../include/pstore/romfs) library. |

### Testing

Executables that are used to test the pstore library.

| Name | Description |
| --- | --- |
| [pstore&#8209;broker&#8209;poker](broker_poker/) | A utility for exercising the broker agent: used by the system tests.  |
| [pstore&#8209;hamt&#8209;test](hamt_test/) | A utility to exercise and verify the behavior of the pstore index implementation. |
| [pstore&#8209;httpd](httpd/) | A minimal HTTP server to exercise the [httpd](../include/pstore/httpd) library. |
| [pstore&#8209;inserter](inserter/) | A utility to exercise the digest index. |
| [pstore&#8209;json](json/) | A small wrapper for the [JSON parser](../include/pstore/json) library. |
| [pstore-lock-test](lock_test/) | A test for the cross-process transaction lock. |
| [pstore&#8209;mangle](mangle/) | A simple file fuzzing utility. |
| [pstore&#8209;sieve](sieve/) | A utility to generate data for the system tests. |
