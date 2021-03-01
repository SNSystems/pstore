#===- unittests/support/CMakeLists.txt ------------------------------------===//
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

add_pstore_unit_test (pstore-support-unit-tests
    test_aligned.cpp
    test_base64.cpp
    test_bit_count.cpp
    test_bit_field.cpp
    test_error.cpp
    test_fnv.cpp
    test_gsl.cpp
    test_maybe.cpp
    test_parallel_for_each.cpp
    test_pointee_adaptor.cpp
    test_quoted.cpp
    test_round2.cpp
    test_uint128.cpp
    test_unsigned_cast.cpp
    test_utf.cpp
    test_utf_win32.cpp
    test_varint.cpp
)
target_link_libraries (pstore-support-unit-tests
    PRIVATE
        pstore-support
        pstore-unit-test-common
)

add_subdirectory (klee EXCLUDE_FROM_ALL)
