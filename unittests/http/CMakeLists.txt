#===- unittests/http/CMakeLists.txt ---------------------------------------===//
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
add_pstore_unit_test (pstore-http-unit-tests
    buffered_reader_mocks.cpp
    buffered_reader_mocks.hpp

    test_buffered_reader.cpp
    test_error_reporting.cpp
    test_headers.cpp
    test_media_type.cpp
    test_query_to_kvp.cpp
    test_request.cpp
    test_serve_dynamic_content.cpp
    test_serve_static_content.cpp
    test_wskey.cpp
    test_ws_server.cpp
)
target_link_libraries (pstore-http-unit-tests PRIVATE pstore-http)

add_subdirectory (klee EXCLUDE_FROM_ALL)
