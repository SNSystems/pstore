#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/broker/CMakeLists.txt -------------------------------------------===//
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

set (BROKER_PUBLIC_INCLUDES_DIR "${PSTORE_ROOT_DIR}/include")

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
    ESCAPE_QUOTES
)
set (PSTORE_BROKER_LIB_SRC
    "${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"

    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/bimap.hpp"
    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/command.hpp"
    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/gc.hpp"
    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/globals.hpp"
    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/intrusive_list.hpp"
    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/message_pool.hpp"
    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/message_queue.hpp"
    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/parser.hpp"
    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/pointer_compare.hpp"
    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/quit.hpp"
    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/read_loop.hpp"
    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/recorder.hpp"
    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/scavenger.hpp"
    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/spawn.hpp"
    "${BROKER_PUBLIC_INCLUDES_DIR}/broker/status_server.hpp"

    command.cpp
    gc_common.cpp
    gc_posix.cpp
    gc_win32.cpp
    globals.cpp
    message_pool.cpp
    parser.cpp
    quit.cpp
    read_loop_posix.cpp
    read_loop_win32.cpp
    recorder.cpp
    scavenger.cpp
    spawn_posix.cpp
    spawn_win32.cpp
    status_server.cpp
)

include (add_pstore)
if (PSTORE_IS_INSIDE_LLVM)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)

    add_pstore_library (pstore-broker-lib STATIC
        ${PSTORE_BROKER_LIB_SRC}
        DEPENDS
        pstore
    )
    include_directories(BEFORE
        ${BROKER_PUBLIC_INCLUDES_DIR}
        ${CMAKE_CURRENT_BINARY_DIR})

    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
else ()
    add_pstore_library (pstore-broker-lib ${PSTORE_BROKER_LIB_SRC})
    target_include_directories (pstore-broker-lib PUBLIC ${BROKER_PUBLIC_INCLUDES_DIR})
endif (PSTORE_IS_INSIDE_LLVM)

target_link_libraries (pstore-broker-lib PUBLIC pstore-broker-intf pstore-support-lib)

# eof: lib/broker/CMakeLists.txt
