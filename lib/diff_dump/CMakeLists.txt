#===- lib/diff_dump/CMakeLists.txt ----------------------------------------===//
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

##########################
# pstore diff-dump library
##########################

add_pstore_library (
    TARGET pstore-diff-dump
    NAME diff-dump
    SOURCES
        revision.cpp
        diff_value.cpp
    HEADER_DIR
        "${PSTORE_ROOT_DIR}/include/pstore/diff_dump"
    INCLUDES
        revision.hpp
        diff_value.hpp
)
target_link_libraries (pstore-diff-dump PUBLIC pstore-adt pstore-core pstore-dump-lib)
add_clang_tidy_target (pstore-diff-dump)
