#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- tools/genromfs/CMakeLists.txt ---------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#===----------------------------------------------------------------------===//
add_pstore_executable (pstore-genromfs
    genromfs.cpp

    copy.cpp
    copy.hpp
    directory_entry.hpp
    dump_tree.cpp
    dump_tree.hpp
    indent.hpp
    scan.cpp
    scan.hpp
    vars.cpp
    vars.hpp
)
target_link_libraries (pstore-genromfs PRIVATE pstore-romfs pstore-command-line)
add_clang_tidy_target (pstore-genromfs)
run_pstore_unit_test (pstore-genromfs pstore-romfs-unit-tests)
