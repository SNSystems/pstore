#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/core/CMakeLists.txt ---------------------------------------------===//
# Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
set (pstore_core_core_includes
    "${pstore_core_include_dir}/address.hpp"
    "${pstore_core_include_dir}/database.hpp"
    "${pstore_core_include_dir}/db_archive.hpp"
    "${pstore_core_include_dir}/file_header.hpp"
    "${pstore_core_include_dir}/generation_iterator.hpp"
    "${pstore_core_include_dir}/index_types.hpp"
    "${pstore_core_include_dir}/indirect_string.hpp"
    "${pstore_core_include_dir}/region.hpp"
    "${pstore_core_include_dir}/start_vacuum.hpp"
    "${pstore_core_include_dir}/storage.hpp"
    "${pstore_core_include_dir}/transaction.hpp"
    "${pstore_core_include_dir}/vacuum_intf.hpp"
)
list (APPEND pstore_core_includes ${pstore_core_core_includes})
set (pstore_core_core_src
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
list (APPEND PSTORE_SRC ${pstore_core_core_src})
source_group ("core" FILES ${pstore_core_core_src} ${pstore_core_core_includes})


###########
# Support #
###########
set (pstore_core_support_includes
    "${pstore_core_include_dir}/array_stack.hpp"
    "${pstore_core_include_dir}/crc32.hpp"
    "${pstore_core_include_dir}/sstring_view_archive.hpp"
    "${pstore_core_include_dir}/time.hpp"
    "${pstore_core_include_dir}/uuid.hpp"
)
list (APPEND pstore_core_includes ${pstore_core_support_includes})
set (pstore_core_support_src
    base32.cpp
    base32.hpp
    crc32.cpp
    time.cpp
    uuid.cpp
)
list (APPEND PSTORE_SRC ${pstore_core_support_src})
source_group ("support" FILES ${pstore_core_support_src} ${pstore_core_support_includes})


######
# OS #
######
set (pstore_core_os_includes
    "${pstore_core_include_dir}/errno_exception.hpp"
)
list (APPEND pstore_core_includes ${pstore_core_os_includes})
set (pstore_core_os_src
    errno_exception.cpp
)
list (APPEND PSTORE_SRC ${pstore_core_os_src})
source_group (os FILES ${pstore_core_os_src} ${pstore_core_os_includes})


#########
# Index #
#########
set (pstore_core_index_includes
    "${pstore_core_include_dir}/hamt_map.hpp"
    "${pstore_core_include_dir}/hamt_map_fwd.hpp"
    "${pstore_core_include_dir}/hamt_map_types.hpp"
    "${pstore_core_include_dir}/hamt_set.hpp"
)
list (APPEND pstore_core_includes ${pstore_core_index_includes})
set (PSTORE_INDEX_SRC
    hamt_map_types.cpp
)
list (APPEND PSTORE_SRC ${PSTORE_INDEX_SRC})
source_group (index FILES ${PSTORE_INDEX_SRC} ${pstore_core_index_includes})



include (add_pstore)
add_pstore_library (
    NAME core
    SOURCES ${PSTORE_SRC}
    INCLUDES ${pstore_core_includes}
)


# Our use of shared memory needs an extra library on a Linux system.
include (CheckLibraryExists)
check_library_exists (rt shm_open "" NEED_LIBRT)
if (NEED_LIBRT)
    target_link_libraries (pstore-core PRIVATE rt)
endif ()


target_link_libraries (pstore-core PUBLIC pstore-broker-intf pstore-os pstore-serialize pstore-support)
add_clang_tidy_target (pstore-core)


include (check_atomic)
if (NOT PSTORE_HAVE_CXX_ATOMICS_WITHOUT_LIB AND PSTORE_HAVE_CXX_ATOMICS_WITH_LIB)
    target_link_libraries (pstore-core PRIVATE atomic)
endif()
