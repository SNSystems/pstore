#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/pstore_broker_intf/CMakeLists.txt -------------------------------===//
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

set (PSTORE_INCLUDE_DIR "${PSTORE_ROOT_DIR}/include")

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/broker_intf_config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/broker_intf_config.hpp"
    ESCAPE_QUOTES
)

set (PSTORE_BROKER_INTF_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/broker_intf_config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/broker_intf_config.hpp"
    "${PSTORE_INCLUDE_DIR}/pstore_broker_intf/fifo_path.hpp"
    "${PSTORE_INCLUDE_DIR}/pstore_broker_intf/message_type.hpp"
    "${PSTORE_INCLUDE_DIR}/pstore_broker_intf/send_message.hpp"
    "${PSTORE_INCLUDE_DIR}/pstore_broker_intf/unique_handle.hpp"
    "${PSTORE_INCLUDE_DIR}/pstore_broker_intf/writer.hpp"
    fifo_path_common.cpp
    fifo_path_posix.cpp
    fifo_path_win32.cpp
    message_type.cpp
    send_message.cpp
    writer_common.cpp
    writer_posix.cpp
    writer_win32.cpp
)

include (add_pstore)
if (PSTORE_IS_INSIDE_LLVM)
    add_pstore_library (pstore-broker-intf STATIC ${PSTORE_BROKER_INTF_SRC})
    target_link_libraries (pstore-broker-intf PUBLIC pstore-support-lib)

    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)

    add_pstore_library (pstore-broker-intf-ex STATIC ${PSTORE_BROKER_INTF_SRC})
    target_link_libraries (pstore-broker-intf-ex PUBLIC pstore-support-lib-ex)

    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)

    include_directories (BEFORE "${PSTORE_INCLUDE_DIR}")

else ()
    add_pstore_library (pstore-broker-intf ${PSTORE_BROKER_INTF_SRC})
    target_include_directories (pstore-broker-intf PUBLIC "${PSTORE_INCLUDE_DIR}")
    target_link_libraries (pstore-broker-intf PUBLIC pstore-support-lib)
endif (PSTORE_IS_INSIDE_LLVM)

# eof: lib/pstore_broker_intf/CMakeLists.txt
