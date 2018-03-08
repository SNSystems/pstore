# pstore Tools

### Garbage Collection

| Name | Description |
| --- | --- |
| [pstore-brokerd](broker/) | The pstore message broker. The broker is a service which is used to manage the garbage collection processes. |
| [pstore-vacuumd](vacum/) | Data store garbage collector. |

### Exploring

| Name | Description |
| --- | --- |
| [pstore-diff](diff/) | Dumps diff between two pstore revisions. |
| [pstore-dump](dump/) | Dumps pstore contents as YAML. |
| [pstore-index-structure](index_structure/) | Dumps pstore index structures as GraphViz DOT graphs. |

The utility programs in this group enable you to inspect and explore the contents of a pstore database.

### Read and Write

| Name | Description |
| --- | --- |
| [pstore-read](read/) | A utility for reading the write or strings index. |
| [pstore-write](write/) | A utility for writing to the write or strings index. |

These utilities are included purely to provide a simple means of experimenting with the pstore library. `pstore-write` enables you to store specific key/value pairs or whole files in a database (where the key is the file path and the value is the file's contents). `pstore-read`, on the other hand, allows the data that is recorded by `pstore-write` to be extracted by printing it to stdout.

### Testing

| Name | Description |
| --- | --- |
| [pstore-broker-poker](broker_poker/) | A utility for exercising the broker agent: used by the system tests.  |
| [pstore-hamt-test](hamt_test/) | A utility to exercice and verify the behavior of the pstore index implementation. |
| [pstore-inserter](inserter/) | A utility to exercise the digest index. |
| [pstore-json](json/) | A small wrapper for the JSON parser library. |
| [pstore-mangle](mangle/) | A simple file fuzzing utility. |
| [pstore-sieve](sieve/) | A utility to generate data for the system tests. |

