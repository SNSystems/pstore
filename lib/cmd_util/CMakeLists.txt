#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/cmd_util/CMakeLists.txt -----------------------------------------===//
# Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
set (PSTORE_CMD_UTIL_INCLUDE_DIR "${PSTORE_ROOT_DIR}/include/pstore/cmd_util")

include (add_pstore)
set (PSTORE_CMD_UTIL_SRC )
set (PSTORE_CMD_UTIL_CL_SRC
    "${PSTORE_CMD_UTIL_INCLUDE_DIR}/cl/category.hpp"
    "${PSTORE_CMD_UTIL_INCLUDE_DIR}/cl/command_line.hpp"
    "${PSTORE_CMD_UTIL_INCLUDE_DIR}/cl/help.hpp"
    "${PSTORE_CMD_UTIL_INCLUDE_DIR}/cl/modifiers.hpp"
    "${PSTORE_CMD_UTIL_INCLUDE_DIR}/cl/option.hpp"
    "${PSTORE_CMD_UTIL_INCLUDE_DIR}/cl/parser.hpp"
    "${PSTORE_CMD_UTIL_INCLUDE_DIR}/cl/string_distance.hpp"
    "${PSTORE_CMD_UTIL_INCLUDE_DIR}/cl/word_wrapper.hpp"
    cl/category.cpp
    cl/command_line.cpp
    cl/help.cpp
    cl/modifiers.cpp
    cl/option.cpp
    cl/parser.cpp
    cl/string_distance.cpp
    cl/word_wrapper.cpp
)
list (APPEND PSTORE_CMD_UTIL_SRC ${PSTORE_CMD_UTIL_CL_SRC})
source_group ("cl" FILES ${PSTORE_CMD_UTIL_CL_SRC})

set (PSTORE_CMD_UTIL_OPT_HELPERS_SRC
    "${PSTORE_CMD_UTIL_INCLUDE_DIR}/str_to_revision.hpp"
    "${PSTORE_CMD_UTIL_INCLUDE_DIR}/revision_opt.hpp"
    str_to_revision.cpp
    revision_opt.cpp
)
list (APPEND PSTORE_CMD_UTIL_SRC ${PSTORE_CMD_UTIL_OPT_HELPERS_SRC})
source_group ("opt_helpers" FILES ${PSTORE_CMD_UTIL_OPT_HELPERS_SRC})


list (APPEND PSTORE_CMD_UTIL_SRC
    "${PSTORE_CMD_UTIL_INCLUDE_DIR}/iota_generator.hpp"
    "${PSTORE_CMD_UTIL_INCLUDE_DIR}/parallel_for_each.hpp"
)
add_pstore_library (pstore-cmd-util STATIC ${PSTORE_CMD_UTIL_SRC})
target_link_libraries (pstore-cmd-util PUBLIC pstore-support)

if (NOT PSTORE_EXCEPTIONS)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)

    add_pstore_library (pstore-cmd-util-ex STATIC ${PSTORE_CMD_UTIL_SRC})

    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
    set (PSTORE_EXCEPTIONS No)

    target_link_libraries (pstore-cmd-util-ex pstore-support-ex)
endif (NOT PSTORE_EXCEPTIONS)
