#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- unittests/pstore/CMakeLists.txt -------------------------------------===//
# Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
# All rights reserved.
#
# Developed by:
#   Toolchain Team
#   SN Systems, Ltd.
#   www.snsystems.com
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal with the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimers.
#
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimers in the
#   documentation and/or other materials provided with the distribution.
#
# - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
#   Inc. nor the names of its contributors may be used to endorse or
#   promote products derived from this Software without specific prior
#   written permission.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
#===----------------------------------------------------------------------===//

# The pstore unit test sources
add_pstore_unit_test (pstore-unit-tests
    binary.hpp
    check_for_error.hpp
    empty_store.cpp
    empty_store.hpp
    test_address.cpp
    test_array_stack.cpp
    test_base32.cpp
    test_basic_logger.cpp
    test_bit_count.cpp
    test_crc32.cpp
    test_database.cpp
    test_db_archive.cpp
    test_file_handle.cpp
    test_file_header.cpp
    test_fnv.cpp
    test_generation_iterator.cpp
    test_heartbeat.cpp
    test_hamt_map.cpp
    test_hamt_set.cpp
    test_make_unique.cpp
    test_memory_mapper.cpp
    test_path.cpp
    test_process_file_name.cpp
    test_region.cpp
    test_rotating_log.cpp
    test_shared_memory.cpp
    test_sstring_view.cpp
    test_sstring_view_archive.cpp
    test_sync.cpp
    test_transaction.cpp
    test_two_connections.cpp
    test_uint128.cpp
    test_uuid.cpp
    test_varint.cpp
)
target_link_libraries (pstore-unit-tests pstore)

# Access the library's private include directory.
target_include_directories (pstore-unit-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/pstore")

add_subdirectory (serialize)
add_subdirectory (broker_intf)

#eof:pstore/unittests/pstore/CMakeLists.txt

# eof: unittests/pstore/CMakeLists.txt
