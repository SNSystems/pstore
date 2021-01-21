#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- unittests/core/CMakeLists.txt ---------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#===----------------------------------------------------------------------===//

# The pstore unit test sources
add_pstore_unit_test (pstore-core-unit-tests
    leak_check_fixture.hpp
    test_address.cpp
    test_array_stack.cpp
    test_base32.cpp
    test_basic_logger.cpp
    test_crc32.cpp
    test_database.cpp
    test_db_archive.cpp
    test_diff.cpp
    test_generation_iterator.cpp
    test_hamt_map.cpp
    test_hamt_set.cpp
    test_heartbeat.cpp
    test_indirect_string.cpp
    test_protect.cpp
    test_region.cpp
    test_rotating_log.cpp
    test_sstring_view_archive.cpp
    test_storage.cpp
    test_sync.cpp
    test_transaction.cpp
    test_two_connections.cpp
    test_uuid.cpp
)
target_link_libraries (pstore-core-unit-tests PRIVATE pstore-core pstore-unit-test-common)

# Access the library's private include directory.
target_include_directories (pstore-core-unit-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/core")
