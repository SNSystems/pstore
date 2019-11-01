#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- tools/brokerd/CMakeLists.txt ----------------------------------------===//
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

if (NOT PSTORE_ENABLE_BROKER)
    message (STATUS "pstore broker tool is excluded (PSTORE_ENABLE_BROKER)")
else ()

    include (add_pstore)
    include (clang_tidy)

    ###########################
    # pstore-brokerd Executable
    ###########################

    set (old_llvm_requires_eh ${LLVM_REQUIRES_EH})
    set (old_llvm_requires_rtti ${LLVM_REQUIRES_RTTI})
    set (old_pstore_exceptions ${PSTORE_EXCEPTIONS})

    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)

    set (pstore_http_fs_source "${CMAKE_CURRENT_BINARY_DIR}/fs.cpp")

    add_pstore_tool (pstore-brokerd
        main.cpp
        switches.hpp
        switches.cpp
        "${pstore_http_fs_source}"
    )

    set (LLVM_REQUIRES_EH ${old_llvm_requires_eh})
    set (LLVM_REQUIRES_RTTI ${old_llvm_requires_rtti})
    set (PSTORE_EXCEPTIONS ${old_pstore_exceptions})

    if (PSTORE_EXCEPTIONS)
        target_link_libraries (pstore-brokerd PRIVATE pstore-cmd-util)
    else ()
        target_link_libraries (pstore-brokerd PRIVATE pstore-cmd-util-ex)
    endif ()
    target_link_libraries (pstore-brokerd PRIVATE pstore-broker)

    add_clang_tidy_target (pstore-broker)
    run_pstore_unit_test (pstore-brokerd pstore-broker-unit-tests)

    # Run pstore-genromfs to create the structures for the HTTP server's built-in file system.
    add_custom_command (
        COMMENT "genromfs from ${CMAKE_CURRENT_SOURCE_DIR}/html to ${pstore_http_fs_source}"
        COMMAND pstore-genromfs --var "::fs" "${CMAKE_CURRENT_SOURCE_DIR}/html" > "${pstore_http_fs_source}"
        OUTPUT "${pstore_http_fs_source}"
        DEPENDS pstore-genromfs
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        VERBATIM
    )

    add_subdirectory (service)
endif ()
