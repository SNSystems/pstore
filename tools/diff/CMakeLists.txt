#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- tools/diff/CMakeLists.txt -------------------------------------------===//
# Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
    "${CMAKE_CURRENT_SOURCE_DIR}/diff_config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/diff_config.hpp"
    ESCAPE_QUOTES
)

add_pstore_executable (pstore-diff
    "${CMAKE_CURRENT_BINARY_DIR}/diff_config.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/diff_config.hpp.in"
    main.cpp
    switches.cpp
    switches.hpp
)

target_include_directories (pstore-diff PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries (pstore-diff pstore-diff-lib pstore-cmd-util)
if (PSTORE_IS_INSIDE_LLVM)
    target_link_libraries (pstore-diff LLVMSupport)

endif (PSTORE_IS_INSIDE_LLVM)


add_custom_command (TARGET pstore-diff
    PRE_LINK
    COMMAND pstore-diff-unit-tests "--gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/diff.xml"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running pstore diff unit tests"
    VERBATIM
)
add_dependencies (pstore-diff pstore-diff-unit-tests)

# eof: tools/diff/CMakeLists.txt
