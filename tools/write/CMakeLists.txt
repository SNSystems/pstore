#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- tools/write/CMakeLists.txt ------------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//

include (add_pstore)
add_pstore_tool (pstore-write
    error.cpp
    error.hpp
    main.cpp
    switches.cpp
    switches.hpp
    to_value_pair.cpp
    to_value_pair.hpp
)
target_link_libraries (pstore-write PRIVATE pstore-core pstore-command-line)

run_pstore_unit_test (pstore-write pstore-adt-unit-tests)
run_pstore_unit_test (pstore-write pstore-command-line-unit-tests)
run_pstore_unit_test (pstore-write pstore-core-unit-tests)
run_pstore_unit_test (pstore-write pstore-mcrepo-unit-tests)
run_pstore_unit_test (pstore-write pstore-os-unit-tests)
run_pstore_unit_test (pstore-write pstore-serialize-unit-tests)
run_pstore_unit_test (pstore-write pstore-support-unit-tests)
