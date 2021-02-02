#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- unittests/klee/CMakeLists.txt ---------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//
include (klee)

pstore_can_klee (can_klee)
if (can_klee)
    set (target pstore-klee-cxxstdlib)
    add_library (${target}-bc OBJECT cxxstdlib.cpp)
    target_compile_options (${target}-bc PRIVATE -emit-llvm)
    set_target_properties (${target}-bc PROPERTIES
        CXX_STANDARD ${pstore_cxx_version}
        CXXSTANDARD_REQUIRED Yes
    )
    pstore_configure_klee_test_target (${target}-bc)
endif (can_klee)
