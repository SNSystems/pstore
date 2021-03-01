#===- unittests/os/CMakeLists.txt -----------------------------------------===//
#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===----------------------------------------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//
include (add_pstore)
set (PSTORE_OS_UNIT_TEST_SRC
    test_file.cpp
    test_file_handle.cpp
    test_memory_mapper.cpp
    test_path.cpp
    test_process_file_name.cpp
    test_shared_memory.cpp
)
add_pstore_unit_test (pstore-os-unit-tests ${PSTORE_OS_UNIT_TEST_SRC})
target_link_libraries (pstore-os-unit-tests
    PRIVATE
        pstore-os
        pstore-unit-test-common
)
add_subdirectory (klee EXCLUDE_FROM_ALL)
