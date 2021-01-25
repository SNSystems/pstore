#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- unittests/adt/CMakeLists.txt ----------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//
add_pstore_unit_test (pstore-adt-unit-tests
    test_chunked_vector.cpp
    test_error_or.cpp
    test_small_vector.cpp
    test_sstring_view.cpp
)
target_link_libraries (pstore-adt-unit-tests PRIVATE pstore-adt)
  
