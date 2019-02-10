#*                               _                               _ _    *
#*  _ __ _   _ _ __    _ __  ___| |_ ___  _ __ ___   _   _ _ __ (_) |_  *
#* | '__| | | | '_ \  | '_ \/ __| __/ _ \| '__/ _ \ | | | | '_ \| | __| *
#* | |  | |_| | | | | | |_) \__ \ || (_) | | |  __/ | |_| | | | | | |_  *
#* |_|   \__,_|_| |_| | .__/|___/\__\___/|_|  \___|  \__,_|_| |_|_|\__| *
#*                    |_|                                               *
#*  _            _    *
#* | |_ ___  ___| |_  *
#* | __/ _ \/ __| __| *
#* | ||  __/\__ \ |_  *
#*  \__\___||___/\__| *
#*                    *
#===- cmake/run_pstore_unit_test.cmake ------------------------------------===//
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
if (PSTORE_VALGRIND)
    find_program (VALGRIND_PATH valgrind)
    if (VALGRIND_PATH STREQUAL "VALGRIND_PATH-NOTFOUND")
        set (PSTORE_VALGRIND No)
        message (WARNING "Valgrind was requested but could not be found")
    else ()
        message (STATUS "Running unit tests with Valgrind memcheck")
    endif ()
endif ()


function (run_pstore_unit_test prelink_target test_target)
    if (CMAKE_HOST_SYSTEM_NAME MATCHES "SunOS")
       message (WARNING "Unit tests disabled because of a crash in Google Mock")
    else ()
        set (OUT_XML "${CMAKE_BINARY_DIR}/${test_target}.xml")

	if (PSTORE_VALGRIND)
            add_custom_command (
                TARGET ${prelink_target}
                PRE_LINK
                COMMAND "${VALGRIND_PATH}"
                        --tool=memcheck --leak-check=full --show-reachable=yes
                        --undef-value-errors=yes --track-origins=no
                        --child-silent-after-fork=no --trace-children=no
                        --error-exitcode=13
                        "$<TARGET_FILE:${test_target}>" "--gtest_output=xml:${OUT_XML}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                COMMENT "Valgrind Running ${test_target}"
                DEPENDS ${test_target}
                BYPRODUCTS ${OUT_XML}
                VERBATIM
            )
        elseif (PSTORE_COVERAGE)
            add_custom_command (
                TARGET ${prelink_target}
                PRE_LINK
                COMMAND ${CMAKE_COMMAND}
                        -E env "LLVM_PROFILE_FILE=$<TARGET_FILE:${test_target}>.profraw"
                        "$<TARGET_FILE:${test_target}>"
                        "--gtest_output=xml:${OUT_XML}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                COMMENT "Running ${test_target}"
                DEPENDS ${test_target}
                BYPRODUCTS ${OUT_XML}
                VERBATIM
            )
        else ()
            add_custom_command (
                TARGET ${prelink_target}
                PRE_LINK
                COMMAND "${test_target}" "--gtest_output=xml:${OUT_XML}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                COMMENT "Running ${test_target}"
                DEPENDS ${test_target}
                BYPRODUCTS ${OUT_XML}
                VERBATIM
            )
        endif ()
    endif ()
endfunction (run_pstore_unit_test)
