#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/vacuum/CMakeLists.txt -------------------------------------------===//
# Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
# All rights reserved.
#
# Developed by:
#   Toolchain Team
#   SN Systems, Ltd.
#   www.snsystems.com
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal with the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimers.
#
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimers in the
#   documentation and/or other materials provided with the distribution.
#
# - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
#   Inc. nor the names of its contributors may be used to endorse or
#   promote products derived from this Software without specific prior
#   written permission.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
#===----------------------------------------------------------------------===//

################
# Vacuum Core Library
################

set (PSTORE_INCLUDE_DIR "${PSTORE_ROOT_DIR}/include")

set (PSTORE_VACUUM_LIB_SRC
    "${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"
    "${PSTORE_INCLUDE_DIR}/vacuum/copy.hpp"
    "${PSTORE_INCLUDE_DIR}/vacuum/quit.hpp"
    "${PSTORE_INCLUDE_DIR}/vacuum/status.hpp"
    "${PSTORE_INCLUDE_DIR}/vacuum/watch.hpp"
    "${PSTORE_INCLUDE_DIR}/vacuum/user_options.hpp"
    copy.cpp
    quit.cpp
    watch.cpp
)

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
    ESCAPE_QUOTES
)

include (add_pstore)
if (PSTORE_IS_INSIDE_LLVM)
    add_pstore_library (pstore-vacuum-lib
        ${PSTORE_VACUUM_LIB_SRC}

        DEPENDS
        pstore
    )
else ()
    add_pstore_library (pstore-vacuum-lib ${PSTORE_VACUUM_LIB_SRC})
    target_include_directories (pstore-vacuum-lib PUBLIC "${PSTORE_INCLUDE_DIR}")
    target_include_directories (pstore-vacuum-lib PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
endif (PSTORE_IS_INSIDE_LLVM)
target_link_libraries (pstore-vacuum-lib PUBLIC pstore)

# eof: lib/vacuum/CMakeLists.txt
