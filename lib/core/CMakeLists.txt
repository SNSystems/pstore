#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/core/CMakeLists.txt ---------------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
