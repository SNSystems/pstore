#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/os/CMakeLists.txt -----------------------------------------------===//
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

set (pstore_os_include_dir "${PSTORE_ROOT_DIR}/include/pstore/os")
set (pstore_os_includes
    descriptor.hpp
    file.hpp
    file_posix.hpp
    file_win32.hpp
    logging.hpp
    memory_mapper.hpp
    path.hpp
    process_file_name.hpp
    rotating_log.hpp
    shared_memory.hpp
    signal_cv.hpp
    signal_helpers.hpp
    thread.hpp
    time.hpp
    uint64.hpp
    wsa_startup.hpp
)
set (pstore_os_lib_src
    descriptor.cpp
    file.cpp
    file_posix.cpp
    file_win32.cpp
    logging.cpp
    memory_mapper.cpp
    memory_mapper_posix.cpp
    memory_mapper_win32.cpp
    path.cpp
    process_file_name_posix.cpp
    process_file_name_win32.cpp
    shared_memory.cpp
    signal_cv_posix.cpp
    signal_cv_win32.cpp
    signal_helpers.cpp
    thread.cpp
    thread_posix.cpp
    thread_win32.cpp
    time.cpp
    wsa_startup.cpp
)

add_pstore_library (
    NAME os
    SOURCES "${pstore_os_lib_src}"
    HEADER_DIR "${pstore_os_include_dir}"
    INCLUDES "${pstore_os_includes}"
)
set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package (Threads)

target_link_libraries (pstore-os PUBLIC ${CMAKE_THREAD_LIBS_INIT} pstore-adt pstore-support)
if (WIN32)
    target_link_libraries (pstore-os PUBLIC Ws2_32.dll)
endif (WIN32)
add_clang_tidy_target (pstore-os)


# Add the exception-handling-enabled pstore-os-ex
if (NOT PSTORE_EXCEPTIONS)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)

    add_pstore_library (
        TARGET pstore-os-ex
        NAME os
        SOURCES "${pstore_os_lib_src}"
        HEADER_DIR "${pstore_os_include_dir}"
        INCLUDES "${pstore_os_includes}"
    )
    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
    set (PSTORE_EXCEPTIONS No)

    target_link_libraries (pstore-os-ex PUBLIC ${CMAKE_THREAD_LIBS_INIT} pstore-adt-ex pstore-support-ex)
    if (WIN32)
        target_link_libraries (pstore-os-ex PUBLIC Ws2_32.dll)
    endif (WIN32)
endif (NOT PSTORE_EXCEPTIONS)

if (PSTORE_BITCODE)
    add_pstore_library (
        TARGET pstore-os-bc
        NAME os
        SOURCES "${pstore_os_lib_src}"
        HEADER_DIR "${pstore_os_include_dir}"
        INCLUDES "${pstore_os_includes}"
    )
    target_compile_options (pstore-os-bc PRIVATE
        -emit-llvm
        -target x86_64-pc-linux-gnu
        -fsanitize=signed-integer-overflow
        -fsanitize=unsigned-integer-overflow
        -fno-threadsafe-statics
    )
    target_link_libraries (pstore-os-bc PUBLIC ${CMAKE_THREAD_LIBS_INIT} pstore-adt-bc pstore-support-bc)
endif (PSTORE_BITCODE)
