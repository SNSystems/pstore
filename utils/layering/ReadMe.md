# layering

A utility which verifies the pstore project's layering. That is, it checks that the
dependencies defined by the CMake project files (the `CMakeLists.txt` files) match the dependencies used by the `#include` statements in the C++ source files.

Each target defined by the CMake project has a set of zero or more dependencies defined by commands such as `target_link_libraries`. The directed graph formed by these connections specifies both the order in which the targets must be built by the build system and the set of include files that may be accessed by the target’s source files. This is the project’s “layering”.

A source file that includes a file which is not specified by the CMake layering runs the risk of mysterious build failures. The compiler has no means to check this for us, so this tool ensures that the source files and the CMake files are consistent and correct.


## Dependencies

The utility uses some third-party Python libraries which must be installed before it can be used:

    $ pip install networkx
    $ pip install decorator
    $ pip install pydot

## Using the tool

With the working directory at the project’s root, run CMake with the `--graphviz=xx` option
to produce the project’s dependency graph:

    $ mkdir dep
    $ cd dep
    $ cmake --graphviz=pstore.dot ..
    $ cd -

With the working directory again at the project’s root, run layering.py:

    $ ./utils/layering/layering.py dep/pstore.dot

It will produce output something like:

    WARNING: skipping: "examples/write_using_serializer/write_using_serializer.cpp"
    WARNING: skipping: "examples/serialize/nonpod_multiple/nonpod_multiple.cpp"
    WARNING: skipping: "examples/serialize/istream_reader/istream_reader.cpp"
    WARNING: skipping: "dep/include/pstore/config/config.hpp"
    WARNING: skipping: "examples/serialize/write_pod_struct/write_pod_struct.cpp"
    WARNING: skipping: "examples/serialize/vector_int_reader/vector_int_reader.cpp"
    WARNING: skipping: "examples/serialize/ostream_writer/ostream_writer.cpp"
    WARNING: skipping: "examples/write_basic/write_basic.cpp"
    WARNING: skipping: "examples/serialize/vector_string_reader/vector_string_reader.cpp"
    WARNING: skipping: "examples/serialize/nonpod2/nonpod2.cpp"
    WARNING: skipping: "examples/serialize/nonpod1/nonpod1.cpp"
    WARNING: skipping: "examples/serialize/write_integers/write_integers.cpp"

(The tool doesn't currently understand the `examples/` directory structure; the `dep/` directory contains the CMake GraphViz output.)

If there is a layering violation, you will see an error reflecting this:

    ERROR: cannot include from component "core" from file "lib/support/error.cpp" (component "support-lib")

On success, the exit code is 0 and 1 if there are any errors.
