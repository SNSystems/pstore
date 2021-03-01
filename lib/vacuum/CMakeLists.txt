#===- lib/vacuum/CMakeLists.txt -------------------------------------------===//
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

################
# Vacuum Core Library
################

add_pstore_library (
    TARGET pstore-vacuum-lib
    NAME vacuum
    SOURCES
        copy.cpp
        quit.cpp
        watch.cpp
    HEADER_DIR
        "${PSTORE_ROOT_DIR}/include/pstore/vacuum"
    INCLUDES
        copy.hpp
        quit.hpp
        status.hpp
        watch.hpp
        user_options.hpp
)
target_link_libraries (pstore-vacuum-lib PUBLIC pstore-brokerface pstore-core)
