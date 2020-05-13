#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/adt/CMakeLists.txt ----------------------------------------------===//
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
set (pstore_adt_lib_src
    sstring_view.cpp
)
set (pstore_adt_include_dir "${PSTORE_ROOT_DIR}/include/pstore/adt")
set (pstore_adt_lib_includes
    chunked_vector.hpp
    error_or.hpp
    small_vector.hpp
    sstring_view.hpp
)
      
add_pstore_library (
    NAME adt
    SOURCES ${pstore_adt_lib_src}
    HEADER_DIR "${pstore_adt_include_dir}"
    INCLUDES ${pstore_adt_lib_includes}
)
target_link_libraries (pstore-adt PUBLIC pstore-support)
add_clang_tidy_target (pstore-adt)

# Add the exception-handling-enabled pstore-adt-ex if we're building inside LLVM
if (NOT PSTORE_EXCEPTIONS)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)
    add_pstore_library (
        TARGET pstore-adt-ex
        NAME adt
        SOURCES ${pstore_adt_lib_src}
        HEADER_DIR "${pstore_adt_include_dir}"
        INCLUDES ${pstore_adt_lib_includes}
    )
    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
    set (PSTORE_EXCEPTIONS No)

    target_link_libraries (pstore-adt-ex PUBLIC pstore-support-ex)
endif (NOT PSTORE_EXCEPTIONS)

if (PSTORE_BITCODE)
    add_pstore_library (
        TARGET pstore-adt-bc
        NAME os
        SOURCES ${pstore_adt_lib_src}
        HEADER_DIR "${pstore_adt_include_dir}"
        INCLUDES ${pstore_adt_lib_includes}
    )
    target_compile_options (pstore-os-bc PRIVATE
        -emit-llvm
        -target x86_64-pc-linux-gnu
        -fsanitize=signed-integer-overflow
        -fsanitize=unsigned-integer-overflow
        -fno-threadsafe-statics
    )
    target_link_libraries (pstore-os-bc PUBLIC pstore-support-bc)
endif (PSTORE_BITCODE)
