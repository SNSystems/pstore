#===- lib/http/CMakeLists.txt ---------------------------------------------===//
#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===----------------------------------------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//
include (add_pstore)

set (pstore_http_public_include "${PSTORE_ROOT_DIR}/include/pstore/http")
set (pstore_http_includes
    block_for_input.hpp
    buffered_reader.hpp
    endian.hpp
    error.hpp
    error_reporting.hpp
    headers.hpp
    http_date.hpp
    media_type.hpp
    net_txrx.hpp
    query_to_kvp.hpp
    quit.hpp
    request.hpp
    send.hpp
    serve_dynamic_content.hpp
    serve_static_content.hpp
    server.hpp
    server_status.hpp
    ws_server.hpp
    wskey.hpp
)
set (pstore_http_lib_src
    error.cpp
    error_reporting.cpp
    headers.cpp
    http_date.cpp
    media_type.cpp
    net_txrx.cpp
    quit.cpp
    server.cpp
    server_status.cpp
    ws_server.cpp
    wskey.cpp
    "${pstore_http_fs_source}"
)

add_pstore_library (
    TARGET pstore-http
    NAME http
    SOURCES "${pstore_http_lib_src}"
    HEADER_DIR "${pstore_http_public_include}"
    INCLUDES "${pstore_http_includes}"
)
target_link_libraries (pstore-http PUBLIC
    pstore-adt
    pstore-brokerface
    pstore-json-lib
    pstore-romfs
    pstore-os
    pstore-support
)
add_clang_tidy_target (pstore-http)

if (PSTORE_ENABLE_BROKER AND NOT PSTORE_EXCEPTIONS)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)

    add_pstore_library (
        TARGET pstore-http-ex
        NAME http
        SOURCES "${pstore_http_lib_src}"
        HEADER_DIR "${pstore_http_public_include}"
        INCLUDES "${pstore_http_includes}"
    )

    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
    set (PSTORE_EXCEPTIONS No)

    target_link_libraries (pstore-http-ex PUBLIC
        pstore-adt-ex
        pstore-brokerface-ex
        pstore-json-lib-ex
        pstore-romfs-ex
        pstore-os-ex
        pstore-support-ex
    )
endif (PSTORE_ENABLE_BROKER AND NOT PSTORE_EXCEPTIONS)

if (PSTORE_BITCODE)
    add_pstore_library (
        TARGET pstore-http-bc
        NAME http
        SOURCES "${pstore_http_lib_src}"
        HEADER_DIR "${pstore_http_public_include}"
        INCLUDES "${pstore_http_includes}"
    )
    target_compile_options (pstore-http-bc PRIVATE
        -emit-llvm
        -target x86_64-pc-linux-gnu
        -fsanitize=signed-integer-overflow
        -fsanitize=unsigned-integer-overflow
        -fno-threadsafe-statics
    )
    set_target_properties (pstore-http-bc PROPERTIES
        CXX_STANDARD "${pstore_cxx_version}"
        CXX_STANDARD_REQUIRED Yes
    )
    target_link_libraries (pstore-http-bc PUBLIC
        pstore-brokerface-bc
        pstore-romfs-bc
        pstore-os-bc
        pstore-support-bc
    )
endif (PSTORE_BITCODE)
