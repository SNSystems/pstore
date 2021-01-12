#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/core/CMakeLists.txt ---------------------------------------------===//
# Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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

include (add_pstore)
include (clang_tidy)

set (pstore_core_include_dir "${PSTORE_ROOT_DIR}/include/pstore/core")
set (PSTORE_SRC )

########
# Core #
########
list (APPEND pstore_core_includes
    address.hpp
    database.hpp
    db_archive.hpp
    diff.hpp
    file_header.hpp
    generation_iterator.hpp
    index_types.hpp
    indirect_string.hpp
    region.hpp
    start_vacuum.hpp
    storage.hpp
    transaction.hpp
    vacuum_intf.hpp
)
list (APPEND PSTORE_SRC
    address.cpp
    database.cpp
    file_header.cpp
    generation_iterator.cpp
    heartbeat.cpp
    heartbeat.hpp
    index_types.cpp
    indirect_string.cpp
    region.cpp
    start_vacuum.cpp
    storage.cpp
    transaction.cpp
    vacuum_intf.cpp
)


###########
# Support #
###########
list (APPEND pstore_core_includes
    array_stack.hpp
    crc32.hpp
    sstring_view_archive.hpp
    time.hpp
    uuid.hpp
)
list (APPEND PSTORE_SRC
    base32.cpp
    base32.hpp
    crc32.cpp
    time.cpp
    uuid.cpp
)


######
# OS #
######
list (APPEND pstore_core_includes errno_exception.hpp)
list (APPEND PSTORE_SRC errno_exception.cpp)


#########
# Index #
#########
list (APPEND pstore_core_includes
    hamt_map.hpp
    hamt_map_fwd.hpp
    hamt_map_types.hpp
    hamt_set.hpp
)
list (APPEND PSTORE_SRC
    hamt_map_types.cpp
)


add_pstore_library (
    NAME core
    SOURCES "${PSTORE_SRC}"
    HEADER_DIR "${pstore_core_include_dir}"
    INCLUDES "${pstore_core_includes}"
)



target_link_libraries (pstore-core PUBLIC
    pstore-adt
    pstore-brokerface
    pstore-os
    pstore-serialize
    pstore-support
)
add_clang_tidy_target (pstore-core)


# Our use of shared memory needs an extra library on a Linux system.
include (CheckLibraryExists)
check_library_exists (rt shm_open "" NEED_LIBRT)
if (NEED_LIBRT)
    target_link_libraries (pstore-core PRIVATE rt)
endif ()

include (check_atomic)
if (NOT PSTORE_HAVE_CXX_ATOMICS_WITHOUT_LIB AND PSTORE_HAVE_CXX_ATOMICS_WITH_LIB)
    target_link_libraries (pstore-core PRIVATE atomic)
endif()
