#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/broker_intf/CMakeLists.txt --------------------------------------===//
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

set (pstore_broker_include_dir "${PSTORE_ROOT_DIR}/include/pstore/broker_intf")

set (pstore_broker_intf_includes
    descriptor.hpp
    fifo_path.hpp
    message_type.hpp
    send_message.hpp
    signal_cv.hpp
    writer.hpp
    wsa_startup.hpp
)

set (pstore_broker_intf_src
    descriptor.cpp
    fifo_path_common.cpp
    fifo_path_posix.cpp
    fifo_path_win32.cpp
    message_type.cpp
    send_message.cpp
    signal_cv_posix.cpp
    signal_cv_win32.cpp
    writer_common.cpp
    writer_posix.cpp
    writer_win32.cpp
    wsa_startup.cpp
)

include (add_pstore)
add_pstore_library (TARGET pstore-broker-intf
    NAME broker_intf
    SOURCES "${pstore_broker_intf_src}"
    HEADER_DIR "${pstore_broker_include_dir}"
    INCLUDES "${pstore_broker_intf_includes}"
)
target_link_libraries (pstore-broker-intf PUBLIC pstore-support)

if (NOT PSTORE_EXCEPTIONS)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)

    add_pstore_library (TARGET pstore-broker-intf-ex
        NAME broker-intf-ex
        SOURCES "${pstore_broker_intf_src}"
        HEADER_DIR "${pstore_broker_include_dir}"
        INCLUDES "${pstore_broker_intf_includes}"
    )

    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
    set (PSTORE_EXCEPTIONS No)

    target_link_libraries (pstore-broker-intf-ex PUBLIC pstore-support-ex)
endif (NOT PSTORE_EXCEPTIONS)

if (WIN32)
    target_link_libraries (pstore-broker-intf PUBLIC Ws2_32.dll)
    if (NOT PSTORE_EXCEPTIONS)
        target_link_libraries (pstore-broker-intf-ex PUBLIC Ws2_32.dll)
    endif ()
endif (WIN32)
