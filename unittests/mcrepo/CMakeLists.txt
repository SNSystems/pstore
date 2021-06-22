#===- unittests/mcrepo/CMakeLists.txt -------------------------------------===//
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
add_pstore_unit_test (pstore-mcrepo-unit-tests
    test_bss_section.cpp
    test_compilation.cpp
    test_debug_line_section.cpp
    test_fragment.cpp
    transaction.cpp
    transaction.hpp
)
target_link_libraries (pstore-mcrepo-unit-tests
    PRIVATE
        pstore-mcrepo
        pstore-unit-test-common
)
