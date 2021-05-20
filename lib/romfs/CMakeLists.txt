#===- lib/romfs/CMakeLists.txt --------------------------------------------===//
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

set (pstore_romfs_include_dir "${PSTORE_ROOT_DIR}/include/pstore/romfs")
set (pstore_romfs_includes
    directory.hpp
    dirent.hpp
    romfs.hpp
)
set (pstore_romfs_lib_src
    directory.cpp
    dirent.cpp
    romfs.cpp
)

add_pstore_library (
    NAME romfs
    SOURCES "${pstore_romfs_lib_src}"
    HEADER_DIR "${pstore_romfs_include_dir}"
    INCLUDES "${pstore_romfs_includes}"
)
add_clang_tidy_target (pstore-romfs)
target_link_libraries (pstore-romfs PUBLIC pstore-adt pstore-support)

if (PSTORE_ENABLE_BROKER AND NOT PSTORE_EXCEPTIONS)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)

    add_pstore_library (
        TARGET pstore-romfs-ex
        NAME romfs
        SOURCES "${pstore_romfs_lib_src}"
        HEADER_DIR "${pstore_romfs_include_dir}"
        INCLUDES "${pstore_romfs_includes}"
    )

    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
    set (PSTORE_EXCEPTIONS No)

    target_link_libraries (pstore-romfs-ex PUBLIC pstore-adt-ex pstore-support-ex)
endif (PSTORE_ENABLE_BROKER AND NOT PSTORE_EXCEPTIONS)

if (PSTORE_BITCODE)
    add_pstore_library (
        TARGET pstore-romfs-bc
        NAME romfs
        SOURCES "${pstore_romfs_lib_src}"
        HEADER_DIR "${pstore_romfs_include_dir}"
        INCLUDES "${pstore_romfs_includes}"
    )
    target_compile_options (pstore-romfs-bc PRIVATE
        -emit-llvm
        -target x86_64-pc-linux-gnu
        -fsanitize=signed-integer-overflow
        -fsanitize=unsigned-integer-overflow
        -fno-threadsafe-statics
    )
    set_target_properties (pstore-romfs-bc PROPERTIES
        CXX_STANDARD "${pstore_cxx_version}"
        CXX_STANDARD_REQUIRED Yes
    )
    target_link_libraries (pstore-romfs-bc PUBLIC pstore-support-bc)
endif (PSTORE_BITCODE)
