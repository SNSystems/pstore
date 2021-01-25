#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- examples/serialize/CMakeLists.txt -----------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//

add_subdirectory (istream_reader)
add_subdirectory (nonpod1)
add_subdirectory (nonpod2)
add_subdirectory (nonpod_multiple)
add_subdirectory (ostream_writer)
add_subdirectory (vector_int_reader)
add_subdirectory (vector_string_reader)
add_subdirectory (write_integers)
add_subdirectory (write_pod_struct)

add_custom_target (pstore-examples-serialize
                   ALL
                   DEPENDS
                           example-istream-reader
                           example-nonpod1
                           example-nonpod2
                           example-nonpod-multiple
                           example-ostream-writer
                           example-vector-int-reader
                           example-vector-string-reader
                           example-write-integers
                           example-write-pod-struct
                   COMMENT "Building Serialization Examples"
                   VERBATIM)
