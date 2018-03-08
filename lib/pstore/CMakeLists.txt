#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/pstore/CMakeLists.txt -------------------------------------------===//
# Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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

cmake_minimum_required (VERSION 3.7)

set (PSTORE_PUBLIC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../include/pstore")
set (PSTORE_SRC )
set (PSTORE_CORE_SRC
    "${PSTORE_PUBLIC_INCLUDE_DIR}/address.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/database.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/db_archive.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/file_header.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/generation_iterator.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/index_types.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/head_revision.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/region.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/start_vacuum.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/storage.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/transaction.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/vacuum_intf.hpp"
    address.cpp
    database.cpp
    file_header.cpp
    generation_iterator.cpp
    heartbeat.cpp
    heartbeat.hpp
    index_types.cpp
    region.cpp
    start_vacuum.cpp
    storage.cpp
    transaction.cpp
    vacuum_intf.cpp
)
list (APPEND PSTORE_SRC ${PSTORE_CORE_SRC})
source_group ("core" FILES ${PSTORE_CORE_SRC})

set (PSTORE_SUPPORT_SRC
    "${PSTORE_PUBLIC_INCLUDE_DIR}/array_stack.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/crc32.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/fnv.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/make_unique.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/sstring_view_archive.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/time.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/uint128.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/uuid.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/varint.hpp"
    base32.cpp
    base32.hpp
    crc32.cpp
    fnv.cpp
    time.cpp
    uint128.cpp
    uuid.cpp
)
list (APPEND PSTORE_SRC ${PSTORE_SUPPORT_SRC})
source_group ("support" FILES ${PSTORE_SUPPORT_SRC})

set (PSTORE_OS_SRC
    "${PSTORE_PUBLIC_INCLUDE_DIR}/errno_exception.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/memory_mapper.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/shared_memory.hpp"
    errno_exception.cpp
    memory_mapper.cpp
    memory_mapper_posix.cpp
    memory_mapper_win32.cpp
    shared_memory.cpp
)
list (APPEND PSTORE_SRC ${PSTORE_OS_SRC})
source_group (os FILES ${PSTORE_OS_SRC})

set (PSTORE_INDEX_SRC
    "${PSTORE_PUBLIC_INCLUDE_DIR}/hamt_map.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/hamt_map_fwd.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/hamt_map_types.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/hamt_set.hpp"
    hamt_map_types.cpp
)
list (APPEND PSTORE_SRC ${PSTORE_INDEX_SRC})
source_group (index FILES ${PSTORE_INDEX_SRC})

set (PSTORE_SERIALIZE_SRC
    "${PSTORE_PUBLIC_INCLUDE_DIR}/serialize/archive.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/serialize/common.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/serialize/ios_state.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/serialize/standard_types.hpp"
    "${PSTORE_PUBLIC_INCLUDE_DIR}/serialize/types.hpp"
    serialize/archive.cpp
    serialize/types.cpp
)
list (APPEND PSTORE_SRC "${PSTORE_SERIALIZE_SRC}")
source_group (serialize FILES ${PSTORE_SERIALIZE_SRC})


include (add_pstore)
add_pstore_library (pstore ${PSTORE_SRC})


# Our use of shared memory needs an extra library on a Linux system.
if (UNIX AND NOT APPLE)
    include (CheckLibraryExists)
   	check_library_exists (rt shm_open "" NEED_LIBRT)
    if (NEED_LIBRT)
        target_link_libraries (pstore PRIVATE rt)
    endif ()
endif ()

target_link_libraries (pstore PUBLIC pstore-support-lib pstore-broker-intf)


include (clang-tidy)
if (CLANG_TIDY_FOUND)
    set (CLANG_TIDY_CHECKS -checks=cert-*,misc-*,cppcoreguidelines-*,modernize-*,-cppcoreguidelines-pro-type-union-access,clang-analyzer-nullability*)

    # Collect the *.cpp files: clang-tidy will scan these.
    get_target_property (SOURCE_FILES pstore SOURCES)
    list (FILTER SOURCE_FILES INCLUDE REGEX "cpp")

    # Tell clang-tidy about the target's include directories.
    get_target_property (PSTORE_INCLUDES pstore INCLUDE_DIRECTORIES)
    set (TARGET_INCLUDE "")
    foreach (P ${PSTORE_INCLUDES})
        list (APPEND TARGET_INCLUDE "-I" "${P}")
    endforeach (P)

    add_custom_target(
        pstore-clang-tidy
        COMMAND "${CLANG_TIDY}"
        ${CLANG_TIDY_CHECKS}
        -header-filter="${PROJECT_SOURCE_DIR}/include/.*"
        ${SOURCE_FILES}
        --
        -std=c++14
        -D NDEBUG
        ${TARGET_INCLUDE}
        ${CLANG_TIDY_SYS_INCLUDE}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
else()
    message (WARNING "clang-tidy executable was not found.")
endif()


include (check_atomic)
if (NOT PSTORE_HAVE_CXX_ATOMICS_WITHOUT_LIB AND PSTORE_HAVE_CXX_ATOMICS_WITH_LIB)
    target_link_libraries(pstore PRIVATE atomic)
endif()

# eof: lib/pstore/CMakeLists.txt
