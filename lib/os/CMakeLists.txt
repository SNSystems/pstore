#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/os/CMakeLists.txt -----------------------------------------------===//
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


set (pstore_os_include_dir "${PSTORE_ROOT_DIR}/include/pstore/os")
set (pstore_os_includes
    "${pstore_os_include_dir}/file.hpp"
    "${pstore_os_include_dir}/file_posix.hpp"
    "${pstore_os_include_dir}/file_win32.hpp"
    "${pstore_os_include_dir}/logging.hpp"
    "${pstore_os_include_dir}/memory_mapper.hpp"
    "${pstore_os_include_dir}/process_file_name.hpp"
    "${pstore_os_include_dir}/rotating_log.hpp"
    "${pstore_os_include_dir}/thread.hpp"
    "${pstore_os_include_dir}/uint64.hpp"
)
set (pstore_os_lib_src
    file.cpp
    file_posix.cpp
    file_win32.cpp
    logging.cpp
    memory_mapper.cpp
    memory_mapper_posix.cpp
    memory_mapper_win32.cpp
    process_file_name_posix.cpp
    process_file_name_win32.cpp
    thread.cpp
)

include (add_pstore)
include (clang_tidy)

add_pstore_library (
    NAME os
    SOURCES ${pstore_os_lib_src}
    INCLUDES ${pstore_os_includes}
)
target_link_libraries (pstore-os PUBLIC ${CMAKE_THREAD_LIBS_INIT} pstore-support)
add_clang_tidy_target (pstore-os)

# Add the exception-handling-enabled pstore-os-ex
if (NOT PSTORE_EXCEPTIONS)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)

    add_pstore_library (
        TARGET pstore-os-ex
        NAME os
        SOURCES ${pstore_os_lib_src}
        INCLUDES ${pstore_os_includes}
    )
    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
    set (PSTORE_EXCEPTIONS No)

    target_link_libraries (pstore-os-ex PUBLIC ${CMAKE_THREAD_LIBS_INIT} pstore-support-ex)
endif (NOT PSTORE_EXCEPTIONS)

if (PSTORE_BITCODE)
    add_pstore_library (
        TARGET pstore-os-bc
        NAME os
        SOURCES ${pstore_os_lib_src}
        INCLUDES ${pstore_os_includes}
    )
    target_compile_options (pstore-os-bc PRIVATE
        -emit-llvm
        -target x86_64-pc-linux-gnu
        -fsanitize=signed-integer-overflow
        -fsanitize=unsigned-integer-overflow
        -fno-threadsafe-statics
    )
    target_link_libraries (pstore-os-bc PUBLIC ${CMAKE_THREAD_LIBS_INIT} pstore-os-bc)
endif (PSTORE_BITCODE)
