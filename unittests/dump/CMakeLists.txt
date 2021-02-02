#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- unittests/dump/CMakeLists.txt ---------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//

########################
# pstore Dump Unit Tests
########################

include (add_pstore)
set (PSTORE_DUMP_UNIT_TEST_SRC
    convert.cpp
    convert.hpp
    test_array.cpp
    test_base16.cpp
    test_db.cpp
    test_line_splitter.cpp
    test_mcrepo.cpp
    test_number.cpp
    test_object.cpp
    test_string.cpp
)
add_pstore_unit_test (pstore-dump-unit-tests ${PSTORE_DUMP_UNIT_TEST_SRC})
target_link_libraries (pstore-dump-unit-tests
    PRIVATE
        pstore-dump-lib
        pstore-unit-test-common
)
