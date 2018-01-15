#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- tools/broker_poker/CMakeLists.txt -----------------------------------===//
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

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/poke_config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/poke_config.hpp"
    ESCAPE_QUOTES
)

add_pstore_executable (pstore-broker-poker
    "${CMAKE_CURRENT_SOURCE_DIR}/poke_config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/poke_config.hpp"
    flood_server.cpp
    flood_server.hpp
    poke.cpp
    switches.cpp
    switches.hpp
)
if (PSTORE_IS_INSIDE_LLVM)
    target_link_libraries (pstore-broker-poker LLVMSupport)
endif (PSTORE_IS_INSIDE_LLVM)
target_link_libraries (pstore-broker-poker pstore-broker-intf pstore-cmd-util)
target_include_directories (pstore-broker-poker PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

# eof: tools/broker_poker/CMakeLists.txt
