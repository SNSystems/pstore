#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- unittests/common/CMakeLists.txt -------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#===----------------------------------------------------------------------===//

# The pstore unit test common/shared sources
add_pstore_test_library (pstore-unit-test-common
    check_for_error.hpp
    empty_store.cpp
    empty_store.hpp
    split.hpp
)
target_include_directories (pstore-unit-test-common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries (pstore-unit-test-common PUBLIC pstore-core)
