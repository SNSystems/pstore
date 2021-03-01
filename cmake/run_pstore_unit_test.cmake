#===- cmake/run_pstore_unit_test.cmake ------------------------------------===//
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
#===----------------------------------------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
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
