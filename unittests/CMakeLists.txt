#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- unittests/CMakeLists.txt --------------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#===----------------------------------------------------------------------===//

include (add_pstore)


add_library (pstore-unit-test-harness STATIC harness.cpp)
set_target_properties (pstore-unit-test-harness PROPERTIES FOLDER "pstore test libraries")
set_property (TARGET pstore-unit-test-harness PROPERTY CXX_STANDARD_REQUIRED Yes)
set_property (TARGET pstore-unit-test-harness PROPERTY CXX_STANDARD 14)
add_pstore_additional_compiler_flags (pstore-unit-test-harness)
target_link_libraries (pstore-unit-test-harness PUBLIC gtest gmock)
target_include_directories (pstore-unit-test-harness PRIVATE ../include)
if (PSTORE_IS_INSIDE_LLVM)
    include_directories (${LLVM_MAIN_SRC_DIR}/utils/unittest/googletest/include)
    include_directories (${LLVM_MAIN_SRC_DIR}/utils/unittest/googlemock/include)
    configure_lit_site_cfg (
        ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
        ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
        MAIN_CONFIG
        ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
        )

    list (APPEND PSTORE_TEST_DEPS PstoreUnitTests)

    add_custom_target (pstore-test-depends DEPENDS ${PSTORE_TEST_DEPS})
    set_target_properties (pstore-test-depends PROPERTIES FOLDER "pstore tests")

    add_lit_testsuite (check-pstore "Running the pstore regression tests"
        ${CMAKE_CURRENT_BINARY_DIR}
        PARAMS ${PSTORE_TEST_PARAMS}
        DEPENDS ${PSTORE_TEST_DEPS}
        ARGS ${PSTORE_TEST_EXTRA_ARGS}
        )
    set_target_properties (check-pstore PROPERTIES FOLDER "pstore tests")
endif (PSTORE_IS_INSIDE_LLVM)

add_subdirectory (klee EXCLUDE_FROM_ALL)

add_subdirectory (adt)
add_subdirectory (broker)
add_subdirectory (brokerface)
add_subdirectory (broker_poker)
add_subdirectory (command_line)
add_subdirectory (common)
add_subdirectory (core)
add_subdirectory (diff)
add_subdirectory (dump)
add_subdirectory (exchange)
add_subdirectory (http)
add_subdirectory (json)
add_subdirectory (mcrepo)
add_subdirectory (os)
add_subdirectory (romfs)
add_subdirectory (serialize)
add_subdirectory (support)
add_subdirectory (vacuum)
