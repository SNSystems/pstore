#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- tools/httpd/CMakeLists.txt ------------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#===----------------------------------------------------------------------===//

# Run pstore-genromfs to create the structures for the HTTP server's built-in file system.
set (pstore_http_fs_source "${CMAKE_CURRENT_BINARY_DIR}/fs.cpp")
add_custom_command (
    COMMAND pstore-genromfs "${CMAKE_CURRENT_SOURCE_DIR}/html" > "${pstore_http_fs_source}"
    OUTPUT "${pstore_http_fs_source}"
    DEPENDS pstore-genromfs
)

add_pstore_executable (pstore-httpd
    httpd.cpp
    "${pstore_http_fs_source}"
)
target_link_libraries (pstore-httpd PRIVATE pstore-http pstore-command-line)
run_pstore_unit_test  (pstore-httpd pstore-http-unit-tests)
