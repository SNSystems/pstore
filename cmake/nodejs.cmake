#*                  _       _      *
#*  _ __   ___   __| | ___ (_)___  *
#* | '_ \ / _ \ / _` |/ _ \| / __| *
#* | | | | (_) | (_| |  __/| \__ \ *
#* |_| |_|\___/ \__,_|\___|/ |___/ *
#*                       |__/      *
#===- cmake/nodejs.cmake --------------------------------------------------===//
# Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
function (pstore_split_version_string version_string version_major version_minor version_patch)

    string (REGEX REPLACE "^v?(.*)\n$" "\\1" vs_ "${version_string}")
    string (REPLACE "." ";" version_list_ "${vs_}")
    list (GET version_list_ 0 version_major_)
    list (GET version_list_ 1 version_minor_)
    list (GET version_list_ 2 version_patch_)

    set (${version_major} ${version_major_} PARENT_SCOPE)
    set (${version_minor} ${version_minor_} PARENT_SCOPE)
    set (${version_patch} ${version_patch_} PARENT_SCOPE)

endfunction (pstore_split_version_string)


function (pstore_find_nodejs node_path version_major version_minor version_patch)

    set (${node_path} "${nodepath}-NOTFOUND" PARENT_SCOPE)
    set (${version_major} "" PARENT_SCOPE)
    set (${version_minor} "" PARENT_SCOPE)
    set (${version_patch} "" PARENT_SCOPE)

    find_program (nodejs_executable
        NAMES node nodejs
        DOC "Node.js executable location"
    )

    if (nodejs_executable)
        set (${node_path} "${nodejs_executable}" PARENT_SCOPE)

        # Figure out which version of Node.JS we've got. This will set NODE_VERSION_STRING to the
        # entire version string and node_version_major/minor/patch accordingly.
        execute_process (
            COMMAND ${nodejs_executable} --version
            OUTPUT_VARIABLE _VERSION
            RESULT_VARIABLE node_version_result_
        )
        if (NOT node_version_result_)
            pstore_split_version_string ("${_VERSION}" version_major_ version_minor_ version_patch_)
            message (STATUS "Node.js was found at: ${nodejs_executable} (version ${version_major_}.${version_minor_}.${version_patch_})")
            set (${version_major} ${version_major_} PARENT_SCOPE)
            set (${version_minor} ${version_minor_} PARENT_SCOPE)
            set (${version_patch} ${version_patch_} PARENT_SCOPE)
        else ()
            message (STATUS "Node.js was found at: ${nodejs_executable} (unknown version)")
        endif ()
    else (nodejs_executable)
        message (STATUS "Node.js was NOT found")
    endif (nodejs_executable)

endfunction (pstore_find_nodejs)


function (pstore_find_npm npm_path version_major version_minor version_patch)

    set (${npm_path} "${npm_path}-NOTFOUND" PARENT_SCOPE)
    set (${npm_version_major} "" PARENT_SCOPE)
    set (${npm_version_minor} "" PARENT_SCOPE)
    set (${npm_version_patch} "" PARENT_SCOPE)

    find_program (npm_executable
        NAMES npm.cmd npm
        DOC "NPM executable location"
    )
    if (npm_executable)
        set (${npm_path} "${npm_executable}" PARENT_SCOPE)

        # Figure out which version of NPM we've got.
        execute_process (
            COMMAND ${npm_executable} --version
            OUTPUT_VARIABLE version_
            RESULT_VARIABLE npm_version_result_
        )

        if (NOT npm_version_result_)
            pstore_split_version_string ("${version_}" version_major_ version_minor_ version_patch_)
            message (STATUS "NPM was found at: ${npm_executable} (version ${version_major_}.${version_minor_}.${version_patch_})")
            set (${version_major} ${version_major_} PARENT_SCOPE)
            set (${version_minor} ${version_minor_} PARENT_SCOPE)
            set (${version_patch} ${version_patch_} PARENT_SCOPE)
        else ()
            message (STATUS "NPM was found at: ${nodejs_executable} (unknown version)")
        endif ()
    else (npm_executable)
        message (STATUS "NPM was NOT found")
    endif (npm_executable)

endfunction (pstore_find_npm)
