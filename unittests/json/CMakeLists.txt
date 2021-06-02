#===- unittests/json/CMakeLists.txt ---------------------------------------===//
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
add_pstore_unit_test (pstore-json-unit-tests
    callbacks.cpp
    callbacks.hpp
    test_array.cpp
    test_comment.cpp
    test_json.cpp
    test_number.cpp
    test_object.cpp
    test_string.cpp
)
target_link_libraries (pstore-json-unit-tests PRIVATE pstore-json-lib)

add_subdirectory (klee)
