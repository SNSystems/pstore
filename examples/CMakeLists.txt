#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- examples/CMakeLists.txt ---------------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//
if (NOT PSTORE_EXAMPLES)
    message (STATUS "pstore examples are excluded (PSTORE_EXAMPLES)")
else ()
    add_subdirectory (serialize)

    add_subdirectory (write_basic)
    add_subdirectory (write_using_serializer)

    add_custom_target (pstore-examples
                       ALL
                       DEPENDS
                               pstore-examples-serialize
                               example-write-basic
                               example-write-using-serializer
                       COMMENT "Building pstore Examples"
                       VERBATIM)

endif()
