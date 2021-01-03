#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/romfs/CMakeLists.txt --------------------------------------------===//
# Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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

if (NOT PSTORE_EXCEPTIONS)
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
endif (NOT PSTORE_EXCEPTIONS)

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
