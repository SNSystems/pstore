#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/httpd/CMakeLists.txt --------------------------------------------===//
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
include (add_pstore)

set (pstore_httpd_public_include "${PSTORE_ROOT_DIR}/include/pstore/httpd")
set (pstore_httpd_lib_src
    "${pstore_httpd_public_include}/block_for_input.hpp"
    "${pstore_httpd_public_include}/buffered_reader.hpp"
    "${pstore_httpd_public_include}/endian.hpp"
    "${pstore_httpd_public_include}/error.hpp"
    "${pstore_httpd_public_include}/headers.hpp"
    "${pstore_httpd_public_include}/http_date.hpp"
    "${pstore_httpd_public_include}/media_type.hpp"
    "${pstore_httpd_public_include}/net_txrx.hpp"
    "${pstore_httpd_public_include}/query_to_kvp.hpp"
    "${pstore_httpd_public_include}/quit.hpp"
    "${pstore_httpd_public_include}/request.hpp"
    "${pstore_httpd_public_include}/send.hpp"
    "${pstore_httpd_public_include}/serve_dynamic_content.hpp"
    "${pstore_httpd_public_include}/serve_static_content.hpp"
    "${pstore_httpd_public_include}/server.hpp"
    "${pstore_httpd_public_include}/server_status.hpp"
    "${pstore_httpd_public_include}/ws_server.hpp"
    "${pstore_httpd_public_include}/wskey.hpp"

    error.cpp
    headers.cpp
    http_date.cpp
    media_type.cpp
    net_txrx.cpp
    quit.cpp
    server.cpp
    ws_server.cpp
    wskey.cpp
    "${pstore_httpd_fs_source}"
)
add_pstore_library (pstore-httpd-lib ${pstore_httpd_lib_src})
target_link_libraries (pstore-httpd-lib PUBLIC
    pstore-broker-intf
    pstore-json-lib
    pstore-romfs
    pstore-os
    pstore-support
)
add_clang_tidy_target (pstore-httpd-lib)

if (NOT PSTORE_EXCEPTIONS)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)

    add_pstore_library (pstore-httpd-lib-ex ${pstore_httpd_lib_src})

    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
    set (PSTORE_EXCEPTIONS No)

    target_link_libraries (pstore-httpd-lib-ex PUBLIC
        pstore-broker-intf-ex
        pstore-json-lib-ex
        pstore-romfs-ex
        pstore-os-ex
        pstore-support-ex
    )
endif ()




if (PSTORE_BITCODE)
    add_pstore_library (pstore-httpd-lib-bc ${pstore_httpd_lib_src})
    target_compile_options (pstore-httpd-lib-bc PRIVATE
        -emit-llvm
        -target x86_64-pc-linux-gnu
        -fsanitize=signed-integer-overflow
        -fsanitize=unsigned-integer-overflow
        -fno-threadsafe-statics
    )
    target_link_libraries (pstore-httpd-lib-bc PUBLIC
        pstore-broker-intf-bc
        pstore-romfs-bc
        pstore-os-bc
        pstore-support-bc
    )
endif (PSTORE_BITCODE)
