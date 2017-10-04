#*       _                         _   _     _        *
#*   ___| | __ _ _ __   __ _      | |_(_) __| |_   _  *
#*  / __| |/ _` | '_ \ / _` |_____| __| |/ _` | | | | *
#* | (__| | (_| | | | | (_| |_____| |_| | (_| | |_| | *
#*  \___|_|\__,_|_| |_|\__, |      \__|_|\__,_|\__, | *
#*                     |___/                   |___/  *
#===- CMakeModules/clang-tidy.cmake ---------------------------------------===//
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

set (CLANG_TIDY_SYS_INCLUDE "")
find_program (CLANG_TIDY "clang-tidy")
if (CLANG_TIDY)
    set (CLANG_TIDY_FOUND Yes)

    if (MSVC)
        set (INCLUDE_PATHS "")
    else()
        # Extract the default system include paths from the compiler. We
        # run it with '-v' and those are written to stderr along with
        # other information data doesn't interest us.
        execute_process (
            COMMAND g++ -E -x c++ - -v
            INPUT_FILE "/dev/null"
            ERROR_VARIABLE INCLUDE_PATHS
            OUTPUT_QUIET
        )

        # Remove any text that doesn't relate to the include paths. That's
        # everything after "#include <...> search starts here:" and before
        # "End of search list.". I also remove any macOS framework directories.
        execute_process (
            COMMAND echo "${INCLUDE_PATHS}"
            COMMAND sed "1,/<\\.\\.\\.>/d;/End of search list/,$d;/(framework directory)/d;"
            OUTPUT_VARIABLE  INCLUDE_PATHS
        )
        #message (STATUS "After sed, the include paths are:\n${INCLUDE_PATHS}")
    endif (MSVC)

    # Now turn this string into a Cmake list.
    string (REPLACE "\n" ";" INCLUDE_PATHS "${INCLUDE_PATHS}")

    # Finally we get to build the system include directory
    # switches. Each path has leading and trailing whitespace stripped
    # and an -isystem switch preceeding it.
    foreach (P ${INCLUDE_PATHS})
        string (STRIP "${P}" P)
        list (APPEND CLANG_TIDY_SYS_INCLUDE "-isystem" "${P}")
    endforeach (P)
else ()
    set (CLANG_TIDY_FOUND No)
endif ()
# eof: CMakeModules/clang-tidy.cmake
