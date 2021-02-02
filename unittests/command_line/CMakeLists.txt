#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- unittests/command_line/CMakeLists.txt -------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//

add_pstore_unit_test (pstore-command-line-unit-tests
    test_cl_parser.cpp
    test_command_line.cpp
    test_csv.cpp
    test_help.cpp
    test_modifiers.cpp
    test_str_to_revision.cpp
    test_string_distance.cpp
    test_word_wrapper.cpp
)
target_link_libraries (pstore-command-line-unit-tests PRIVATE pstore-command-line)
