# pstore utility programs

A small collection of useful utilities which simplify some common tasks or perform extra checks on the source code.

## Table of Contents

* [Boilerplate](#boilerplate)
* [Exchange Schema](#exchange-schema)
* [Layering](#layering)
* [Make Build](#make-build)
    * [Options](#options)
* [Redundant Includes](#redundant-includes)
* [Write Travis YAML](#write-travis-yaml)

## Boilerplate

See [the notes](boilerplate/) in the boilerplate directory.

## Exchange Schema

See the [the notes](exchange_schema/) in the exchange_schema directory.

## Layering

See [the notes](layering/) in the layering directory.

## Make Build

The `make_build.py` is intended to simplify running [CMake](https://cmake.org). It creates the directory for the build and runs CMake with arguments suitable for the host system.

### Options

Some of the common switches are described in the following table. For a complete list of the command-line switches supported by `make_build.py`, use the `--help` switch.

| Switch | Description |
| ------ | ----------- |
| `‑‑define`,`‑D` | <p>Used to create or update CMake cache entries. This can be used to pass options into the build.</p><p>**Example:** `$ make_build.py -D PSTORE_EXAMPLES=Yes` will enable the pstore example programs.</p>
| `‑‑directory`,`‑o` | <p>The directory in which the CMake build will be created. By default this is a path inside the current directory with the prefix `build_` and a suffix which is derived from the name of the host operating system.</p><p>For example, running `make_build.py` on macOS would place the build in a directory called `./build_mac`; when running it on Linux then name is `./build_linux`;  Windows is `./build_win32`. This approach allows build for different systems to coexist in the same build tree.If you prefer true out-of-tree builds, then use this switch to specific a particular directory for the build.</p><p>**Warning:** that unless you specify `--no-clean` this directory **will be deleted** before CMake is run.</p><p>**Example:** `$ make_build.py -o ~/build` will place the build in a directory named `~/build`, creating if it does not exist and removing anything that was at that location.</p> |
| `‑‑generator`,`‑g` | <p>The name of the build system generator that is passed to CMake. This is normally automatically determined from the host operating system, but you make explicitly override that choice using this switch.</p><p>**Example:** `$ make_build.py -g "Unix Makefiles"` will use CMake's makefile generator regardless of the host operating system.</p> |

## Redundant Includes

See [the notes](redundant_includes/) in the redundant_includes directory.

## Write Travis YAML

