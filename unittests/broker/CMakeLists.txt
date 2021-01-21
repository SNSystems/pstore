#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- unittests/broker/CMakeLists.txt -------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#===----------------------------------------------------------------------===//

if (NOT PSTORE_ENABLE_BROKER)
    message (STATUS "pstore broker unittests is excluded (PSTORE_ENABLE_BROKER)")
else ()

    include (add_pstore)
    add_pstore_unit_test (pstore-broker-unit-tests
        test_bimap.cpp
        test_command.cpp
        test_gc.cpp
        test_intrusive_list.cpp
        test_parser.cpp
        test_spawn.cpp
    )

    # Access the library's private include directory.
    target_include_directories (pstore-broker-unit-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/pstore")

    target_link_libraries (pstore-broker-unit-tests PRIVATE pstore-broker)
endif ()
