#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- tools/write/CMakeLists.txt ------------------------------------------===//
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
    "${CMAKE_CURRENT_SOURCE_DIR}/write_config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/write_config.hpp"
    ESCAPE_QUOTES
)
include (add_pstore)
add_pstore_executable (pstore-write
    "${CMAKE_CURRENT_SOURCE_DIR}/write_config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/write_config.hpp"
    main.cpp
    error.cpp
    error.hpp
    to_value_pair.cpp
    to_value_pair.hpp
    switches.hpp
    switches.cpp
)
target_include_directories (pstore-write PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
if (PSTORE_IS_INSIDE_LLVM)
    target_link_libraries (pstore-write LLVMSupport)
endif (PSTORE_IS_INSIDE_LLVM)
target_link_libraries (pstore-write pstore pstore-cmd-util)



add_custom_command (TARGET pstore-write
    PRE_LINK
    COMMAND pstore-support-unit-tests "--gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/support.xml"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running pstore support unit tests"
    VERBATIM
)
add_custom_command (TARGET pstore-write
    PRE_LINK
    COMMAND pstore-unit-tests "--gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/pstore.xml"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running pstore core unit tests"
    VERBATIM
)
add_custom_command (TARGET pstore-write
    PRE_LINK
    COMMAND pstore-cmd-util-tests "--gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/cmd_util.xml"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running pstore cmd-line utility unit tests"
    VERBATIM
)

# eof: tools/write/CMakeLists.txt
