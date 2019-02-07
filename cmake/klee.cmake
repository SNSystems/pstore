#*  _    _            *
#* | | _| | ___  ___  *
#* | |/ / |/ _ \/ _ \ *
#* |   <| |  __/  __/ *
#* |_|\_\_|\___|\___| *
#*                    *
#===- cmake/klee.cmake ----------------------------------------------------===//
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
execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)

# Convert command output into a CMake list
STRING(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
STRING(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")

function(print_properties)
    message ("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction(print_properties)

function(print_target_properties tgt)
    if(NOT TARGET ${tgt})
      message("There is no target named '${tgt}'")
      return()
    endif()

    foreach (prop ${CMAKE_PROPERTY_LIST})
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop ${prop})
    # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
    if(prop STREQUAL "LOCATION" OR prop MATCHES "^LOCATION_" OR prop MATCHES "_LOCATION$")
        continue()
    endif()
        # message ("Checking ${prop}")
        get_property(propval TARGET ${tgt} PROPERTY ${prop} SET)
        if (propval)
            get_target_property(propval ${tgt} ${prop})
            message ("${tgt} ${prop} = ${propval}")
        endif()
    endforeach(prop)
endfunction(print_target_properties)










set (PSTORE_RUN_KLEE  "${PSTORE_ROOT_DIR}/unittests/support/klee/run_klee.py")

# The suport code assumes that we're using the KLEE Docker container which has
# the sources and libraries in the locations given by the following two variables:
set (PSTORE_KLEE_SRC_DIR "/home/klee/klee_src/include")
set (PSTORE_KLEE_LIB_DIR "/home/klee/klee_build/klee/lib/libkleeRuntest.so")


# pstore_can_klee
# ~~~~~~~~~~~~~~~
# Sets the return variable to a true value if the klee tests can be run. The
# host compiler must be clang, the klee executable must be on the path, and we
# must be using cmake version >= 3.9.

function (pstore_can_klee result)

    set (${result} No PARENT_SCOPE)

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang$")

        # The klee executable must be on the path somewhere.
        find_program (PSTORE_KLEE_PATH klee)
	if (NOT "${PSTORE_KLEE_PATH}" STREQUAL "PSTORE_KLEE_PATH-NOTFOUND")

	    # The klee custom cmake targets requires that we running cmake 3.9
	    # or later.
            if (NOT (${CMAKE_MAJOR_VERSION} EQUAL 3 AND ${CMAKE_MINOR_VERSION} LESS 9))
	       set (${result} Yes PARENT_SCOPE)
            endif ()

	endif ()
    endif ()

endfunction (pstore_can_klee)


# pstore_add_klee_run_all_target
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function (pstore_add_klee_run_all_target bitcode_enabled)

    set (${bitcode_enabled} No PARENT_SCOPE)

    pstore_can_klee (can_klee)
    if (can_klee)
        message (STATUS "KLEE targets are enabled")
        set (${bitcode_enabled} Yes PARENT_SCOPE)
	add_custom_target (pstore-klee-run-all)
    else ()
        message (STATUS "KLEE targets are disabled")
    endif (can_klee)

endfunction (pstore_add_klee_run_all_target)


# pstore_configure_klee_test_target
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Configures a KLEE test target with its includes, compiler switches, and so
# on. This function only performs configurations that are common to both the
# bitcode and executable targets.

function (pstore_configure_klee_test_target name)

    target_include_directories (${name} PRIVATE "${PSTORE_KLEE_SRC_DIR}")
    target_compile_options (${name} PRIVATE "-fno-threadsafe-statics")
    set_target_properties (${name} PROPERTIES
        FOLDER "pstore-klee"
	CXX_STANDARD 11
	CXX_STANDARD_REQUIRED Yes
    )

endfunction (pstore_configure_klee_test_target)


# pstore_add_klee_test
# ~~~~~~~~~~~~~~~~~~~~
function (pstore_add_klee_test )

cmake_parse_arguments (
                      klee_prefix
                      "" # options
                      "CATEGORY;NAME" # one-value keywords
                      "DEPENDS" # multi-value keywords.
                      ${ARGN}
                      )

set (category ${klee_prefix_CATEGORY})
set (name ${klee_prefix_NAME})

    pstore_can_klee (can_klee)
    if (can_klee)
        set (tname_base "pstore-klee-${category}-${name}")

        set (bc_tname "${tname_base}-bc")
        set (exe_tname "${tname_base}-exe")

        # The bitcode library.

        add_library ("${bc_tname}" OBJECT ${name}.cpp)
        pstore_configure_klee_test_target ("${bc_tname}")
        target_compile_options ("${bc_tname}" PRIVATE -emit-llvm)

        foreach (dependent ${klee_prefix_DEPENDS})
            get_target_property (includes ${dependent} INTERFACE_INCLUDE_DIRECTORIES)
            target_include_directories ("${bc_tname}" PUBLIC "${includes}")
        endforeach (dependent)

#        get_target_property (support_includes pstore-support INTERFACE_INCLUDE_DIRECTORIES)
#        target_include_directories ("${bc_tname}" PUBLIC "${support_includes}")

        # The executable.

        add_executable (${exe_tname} ${name}.cpp)
        pstore_configure_klee_test_target (${exe_tname})
        target_compile_definitions (${exe_tname} PRIVATE -DPSTORE_KLEE_RUN)

        set (sanitizers "-fsanitize=undefined,address")
        target_compile_options (${exe_tname} PRIVATE ${sanitizers})
        set_target_properties (${exe_tname} PROPERTIES LINK_FLAGS ${sanitizers})

        target_link_libraries (${exe_tname} PRIVATE "${PSTORE_KLEE_LIB_DIR}")
        
        foreach (dependent ${klee_prefix_DEPENDS})
            target_link_libraries (${exe_tname} PRIVATE ${dependent})
        endforeach (dependent)

	set (link_llvm_lib "")
	foreach (dependent ${klee_prefix_DEPENDS})
	    list (APPEND link_llvm_lib "--link-llvm-lib=$<TARGET_FILE:${dependent}-bc>")
	endforeach (dependent)

        add_custom_target (
            "${tname_base}-run"
            COMMAND klee
                    --libc=uclibc
                    --posix-runtime
                    --only-output-states-covering-new
                    --optimize
                    "${link_llvm_lib}"
                    "$<TARGET_OBJECTS:${bc_tname}>"

            COMMAND "${PSTORE_RUN_KLEE}"
                    # --ktest-tool
                    "$<TARGET_FILE:${exe_tname}>"
                    "$<TARGET_OBJECTS:${bc_tname}>"

            DEPENDS "${bc_tname}"
                    ${exe_tname}

            COMMENT "Running KLEE for '${bc_tname}'"
        )
        set_target_properties ("${tname_base}-run" PROPERTIES FOLDER "pstore-klee")
        foreach (dependent ${klee_prefix_DEPENDS})
            add_dependencies ("${tname_base}-run" "${dependent}-bc")
        endforeach (dependent)
        
        add_dependencies (pstore-klee-run-all ${tname_base}-run)
    endif (can_klee)

endfunction (pstore_add_klee_test)
