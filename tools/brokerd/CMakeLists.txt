#===- tools/brokerd/CMakeLists.txt ----------------------------------------===//
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
        run_broker.cpp
        run_broker.hpp
        switches.cpp
        switches.hpp
        "${pstore_http_fs_source}"
    )

    set (LLVM_REQUIRES_EH ${old_llvm_requires_eh})
    set (LLVM_REQUIRES_RTTI ${old_llvm_requires_rtti})
    set (PSTORE_EXCEPTIONS ${old_pstore_exceptions})

    if (PSTORE_EXCEPTIONS)
        target_link_libraries (pstore-brokerd PRIVATE pstore-command-line)
    else ()
        target_link_libraries (pstore-brokerd PRIVATE pstore-command-line-ex)
    endif ()
    target_link_libraries (pstore-brokerd PRIVATE pstore-broker)

    add_clang_tidy_target (pstore-brokerd)
    run_pstore_unit_test (pstore-brokerd pstore-brokerface-unit-tests)
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
