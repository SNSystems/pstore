#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/brokerface/CMakeLists.txt ---------------------------------------===//
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

set (pstore_broker_include_dir "${PSTORE_ROOT_DIR}/include/pstore/brokerface")

set (pstore_brokerface_includes
    fifo_path.hpp
    message_type.hpp
    pubsub.hpp
    send_message.hpp
    writer.hpp
)

set (pstore_brokerface_src
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
add_pstore_library (TARGET pstore-brokerface
    NAME brokerface
    SOURCES "${pstore_brokerface_src}"
    HEADER_DIR "${pstore_broker_include_dir}"
    INCLUDES "${pstore_brokerface_includes}"
)
target_link_libraries (pstore-brokerface PUBLIC pstore-adt pstore-os pstore-support)

if (NOT PSTORE_EXCEPTIONS)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)

    add_pstore_library (TARGET pstore-brokerface-ex
        NAME brokerface-ex
        SOURCES "${pstore_brokerface_src}"
        HEADER_DIR "${pstore_broker_include_dir}"
        INCLUDES "${pstore_brokerface_includes}"
    )

    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
    set (PSTORE_EXCEPTIONS No)

    target_link_libraries (pstore-brokerface-ex PUBLIC pstore-adt-ex pstore-os-ex pstore-support-ex)
endif (NOT PSTORE_EXCEPTIONS)
