#===- tools/brokerd/service/win32/CMakeLists.txt --------------------------===//
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

if (PSTORE_IS_INSIDE_LLVM)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
endif ()

set (pstore_http_fs_source "${CMAKE_CURRENT_BINARY_DIR}/fs.cpp")

add_pstore_tool (pstore-broker-service
    "${pstore_http_fs_source}"
    ../../run_broker.cpp
    ../../run_broker.hpp
    ../../switches.cpp
    ../../switches.hpp
    broker_service.cpp
    broker_service.hpp
    main.cpp
    service_base.cpp
    service_base.hpp
    service_installer.cpp
    service_installer.hpp
)

if (PSTORE_IS_INSIDE_LLVM)
    target_link_libraries (pstore-broker-service PRIVATE pstore-command-line-ex)
    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
else ()
    target_link_libraries (pstore-broker-service PRIVATE pstore-command-line)
endif ()

target_link_libraries (pstore-broker-service PRIVATE pstore-broker)

# Run pstore-genromfs to create the structures for the HTTP server's built-in file system.
add_custom_command (
    COMMENT "genromfs from ${CMAKE_CURRENT_SOURCE_DIR}/../../html to ${pstore_http_fs_source}"
    COMMAND pstore-genromfs --var "::fs" "${CMAKE_CURRENT_SOURCE_DIR}/../../html" > "${pstore_http_fs_source}"
    OUTPUT "${pstore_http_fs_source}"
    DEPENDS pstore-genromfs
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)
