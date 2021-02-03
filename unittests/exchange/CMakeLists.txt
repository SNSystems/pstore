#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- unittests/exchange/CMakeLists.txt -----------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//
include (add_pstore)
add_pstore_unit_test (pstore-exchange-unit-tests
    add_export_strings.hpp
    compare_external_fixups.hpp
    section_helper.hpp
    test_bss_section.cpp
    test_compilation.cpp
    test_export_emit.cpp
    test_generic_section.cpp
    test_linked_definitions_section.cpp
    test_section_fixups.cpp
    test_terminal.cpp
    test_uuid.cpp
)

# Access the library's private include directory.
target_include_directories (pstore-exchange-unit-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/exchange")
target_link_libraries (pstore-exchange-unit-tests
    PRIVATE
        pstore-exchange
        pstore-unit-test-common
)
