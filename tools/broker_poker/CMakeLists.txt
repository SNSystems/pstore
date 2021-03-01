#===- tools/broker_poker/CMakeLists.txt -----------------------------------===//
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

add_pstore_executable (pstore-broker-poker
    flood_server.cpp
    flood_server.hpp
    iota_generator.hpp
    poke.cpp
    switches.cpp
    switches.hpp
)
target_link_libraries (pstore-broker-poker PRIVATE
    pstore-adt
    pstore-brokerface
    pstore-command-line
)

run_pstore_unit_test (pstore-broker-poker pstore-broker-poker-unit-tests)
