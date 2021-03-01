#===- lib/CMakeLists.txt --------------------------------------------------===//
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
add_subdirectory (support)

add_subdirectory (adt)
add_subdirectory (broker)
add_subdirectory (brokerface)
add_subdirectory (command_line)
add_subdirectory (core)
add_subdirectory (diff_dump)
add_subdirectory (dump)
add_subdirectory (exchange)
add_subdirectory (http)
add_subdirectory (json)
add_subdirectory (mcrepo)
add_subdirectory (os)
add_subdirectory (romfs)
add_subdirectory (serialize)
add_subdirectory (vacuum)
