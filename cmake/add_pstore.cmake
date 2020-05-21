#*            _     _             _                  *
#*   __ _  __| | __| |  _ __  ___| |_ ___  _ __ ___  *
#*  / _` |/ _` |/ _` | | '_ \/ __| __/ _ \| '__/ _ \ *
#* | (_| | (_| | (_| | | |_) \__ \ || (_) | | |  __/ *
#*  \__,_|\__,_|\__,_| | .__/|___/\__\___/|_|  \___| *
#*                     |_|                           *
#===- cmake/add_pstore.cmake ----------------------------------------------===//
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
include (CheckCSourceCompiles)
include (CheckCXXCompilerFlag)

set (pstore_cxx_version "14" CACHE STRING "The version of C++ used by pstore")

macro(add_pstore_subdirectory name)
    if (PSTORE_IS_INSIDE_LLVM)
        add_llvm_subdirectory(PSTORE TOOL ${name})
    endif ()
endmacro()


# disable_warning_if_possible
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Adds a switch to the compile options if supported by the compiler.
#
# target_name: The name of the target to which the compile option should be
#              added.
# flag:        The name of the compiler switch to be added.
# result_var:  The name of the CMake cache variable into which the result of the
#              test will be stored. This should be different for each switch
#              being tested.
function (disable_warning_if_possible target_name flag result_var)
    check_cxx_compiler_flag (${flag} ${result_var})
    if (${${result_var}})
        target_compile_options (${target_name} PRIVATE ${flag})
    endif ()
endfunction()


# pstore_enable_warnings
# ~~~~~~~~~~~~~~~~~~~~~~
function (pstore_enable_warnings)
    cmake_parse_arguments (arg # prefix
        "IS_UNIT_TEST" # options
        "TARGET" # one-value-keywords
        "" # multi-value-keywords
        ${ARGN}
    )
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set (options
            -Weverything
            -Wno-c99-extensions
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-c++14-extensions
            -Wno-exit-time-destructors
            -Wno-global-constructors
            -Wno-padded
            -Wno-weak-vtables
        )
        if (${arg_IS_UNIT_TEST})
            list (APPEND options
                -Wno-unused-member-function
                -Wno-used-but-marked-unused
            )
        endif ()
    elseif (CMAKE_COMPILER_IS_GNUCXX)
        set (options
            -Wall
            -Wextra
            -pedantic
        )
    elseif (MSVC)
        set (options -W4)
    endif ()

    target_compile_options (${arg_TARGET} PRIVATE ${options})

endfunction (pstore_enable_warnings)


########################################
# add_pstore_additional_compiler_flags #
########################################
function (add_pstore_additional_compiler_flags target_name)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if (NOT PSTORE_EXCEPTIONS)
           target_compile_options (${target_name} PRIVATE -fno-exceptions -fno-rtti)
        endif ()

        target_compile_options (${target_name} PRIVATE -Wno-c++14-extensions)

        # TODO: this warning is far too pervasive in clang3.8 but much better in later
        # builds. Only disable for older versions.
        disable_warning_if_possible (${target_name}
            -Wno-nullability-completeness
            PSTORE_CLANG_SUPPORTS_NX
        )
        disable_warning_if_possible (${target_name}
            -Wno-nullability-extension
            PSTORE_CLANG_SUPPORTS_NX
        )
        disable_warning_if_possible (${target_name}
            -Wno-zero-as-null-pointer-constant
            PSTORE_CLANG_SUPPORTS_NPC
        )
        # TODO: this warning is issued (at the time of writing) by gtest. It should only be disabled
        # for that target.
        disable_warning_if_possible (${target_name}
            -Wno-inconsistent-missing-override
            PSTORE_CLANG_SUPPORTS_IMO
        )

        # Disable the "unused lambda capture" warning since we must capture extra variables for MSVC
        # to swallow the code. Remove when VS doesn't require capture.
        disable_warning_if_possible (${target_name}
            -Wno-unused-lambda-capture
            PSTORE_CLANG_SUPPORTS_ULC
        )

        if (PSTORE_COVERAGE)
            target_compile_options (${target_name} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
            target_compile_options (${target_name} PRIVATE -fno-inline-functions)
            target_link_libraries (${target_name} PUBLIC -fprofile-instr-generate -fcoverage-mapping)
        endif (PSTORE_COVERAGE)

    elseif (CMAKE_COMPILER_IS_GNUCXX)

        if (NOT PSTORE_EXCEPTIONS)
           target_compile_options (${target_name} PRIVATE -fno-exceptions -fno-rtti)
        endif ()

        target_compile_options (${target_name} PRIVATE
            -Wall
            -Wextra
            -pedantic
        )

	# A warning telling us that the signature will change in C++17 isn't important right now.
	disable_warning_if_possible (${target_name} -Wno-noexcept-type      PSTORE_GCC_SUPPORTS_EXCEPT_TYPE)
        disable_warning_if_possible (${target_name} -Wno-ignored-attributes PSTORE_GCC_SUPPORTS_IGNORED_ATTRIBUTES)

        if (PSTORE_COVERAGE)
            target_compile_options (${target_name} PRIVATE -fprofile-arcs -ftest-coverage)
            target_link_libraries (${target_name} PUBLIC --coverage)
        endif (PSTORE_COVERAGE)

    elseif (MSVC)

        if (NOT PSTORE_EXCEPTIONS)
            #target_compile_options (${target_name} PRIVATE "/EHs-" "/EHc-" "/GR-")

            # Remove /EHsc (which enables exceptions) from the cmake's default compiler switches.
            string (REPLACE "/EHsc" #match string
                            "" #replace string
                            CMAKE_CXX_FLAGS
                            ${CMAKE_CXX_FLAGS})

            target_compile_options (${target_name} PUBLIC -EHs-c-)
            target_compile_definitions (${target_name} PRIVATE -D_HAS_EXCEPTIONS=0)
        endif (NOT PSTORE_EXCEPTIONS)

        # 4127: conditional expression is constant. We're not yet using C++17
        #       therefore if constexpr is not available.
        # 4146: unary minus applied to unsigned, result still unsigned.
        # 4512: assignment operator could not be generated.
        target_compile_options (${target_name} PRIVATE
            -W4
            -wd4127
            -wd4146
            -wd4512
        )
        target_compile_definitions (${target_name} PRIVATE
            -D_CRT_SECURE_NO_WARNINGS
            -D_ENABLE_ATOMIC_ALIGNMENT_FIX
            -D_ENABLE_EXTENDED_ALIGNED_STORAGE
            -D_SCL_SECURE_NO_WARNINGS
            -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
        )

        if (PSTORE_COVERAGE)
            message (WARNING "PSTORE_COVERAGE is not yet implemented for MSVC")
        endif (PSTORE_COVERAGE)

    endif ()

    # On Windows, we're a "Unicode" app.
    if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
        target_compile_definitions ("${target_name}" PRIVATE -DUNICODE -D_UNICODE)
    endif ()

endfunction(add_pstore_additional_compiler_flags)


#############################
# pstore_set_output_directory
#############################

# Set each output directory according to ${CMAKE_CONFIGURATION_TYPES}.
# Note: Don't set variables CMAKE_*_OUTPUT_DIRECTORY any more,
# or a certain builder, for eaxample, msbuild.exe, would be confused.
function(pstore_set_output_directory target)
  cmake_parse_arguments(ARG "" "BINARY_DIR;LIBRARY_DIR" "" ${ARGN})

  # module_dir -- corresponding to LIBRARY_OUTPUT_DIRECTORY.
  # It affects output of add_library(MODULE).
  if(WIN32 OR CYGWIN)
    # DLL platform
    set(module_dir ${ARG_BINARY_DIR})
  else()
    set(module_dir ${ARG_LIBRARY_DIR})
  endif()
  if(NOT "${CMAKE_CFG_INTDIR}" STREQUAL ".")
    foreach(build_mode ${CMAKE_CONFIGURATION_TYPES})
      string(TOUPPER "${build_mode}" CONFIG_SUFFIX)
      if(ARG_BINARY_DIR)
        string(REPLACE ${CMAKE_CFG_INTDIR} ${build_mode} bi ${ARG_BINARY_DIR})
        set_target_properties(${target} PROPERTIES "RUNTIME_OUTPUT_DIRECTORY_${CONFIG_SUFFIX}" ${bi})
      endif()
      if(ARG_LIBRARY_DIR)
        string(REPLACE ${CMAKE_CFG_INTDIR} ${build_mode} li ${ARG_LIBRARY_DIR})
        set_target_properties(${target} PROPERTIES "ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_SUFFIX}" ${li})
      endif()
      if(module_dir)
        string(REPLACE ${CMAKE_CFG_INTDIR} ${build_mode} mi ${module_dir})
        set_target_properties(${target} PROPERTIES "LIBRARY_OUTPUT_DIRECTORY_${CONFIG_SUFFIX}" ${mi})
      endif()
    endforeach()
  else()
    if(ARG_BINARY_DIR)
      set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ARG_BINARY_DIR})
    endif()
    if(ARG_LIBRARY_DIR)
      set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${ARG_LIBRARY_DIR})
    endif()
    if(module_dir)
      set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${module_dir})
    endif()
  endif()
endfunction()


####################
# add_pstore_library
####################

# HEADER_DIR - The path of the directory containing the library's include files.
# NAME - The name of the directory containing the targets include files.
# TARGET - The name of the target to be created. If omitted, defaults to the NAME value with a
#          "pstore-" prefix. Underscores (_) are replaced with dash (-) to follow the pstore naming
#          convention.
# SOURCES - A list of the library's source files.
# INCLUDES - A list of the library's include files.

function (add_pstore_library)
    cmake_parse_arguments (arg ""
        "TARGET;NAME;HEADER_DIR"
        "SOURCES;INCLUDES"
        ${ARGN}
    )
    if ("${arg_NAME}" STREQUAL "")
        message (SEND_ERROR "NAME argument is empty")
    endif ()
    if ("${arg_SOURCES}" STREQUAL "")
        message (SEND_ERROR "add_pstore_library: no SOURCES were supplied")
    endif ()
    if ("${arg_HEADER_DIR}" STREQUAL "")
        message (SEND_ERROR "add_pstore_library: HEADER_DIR argument was missing")
    endif ()

    if ("${arg_TARGET}" STREQUAL "")
       set (arg_TARGET "pstore-${arg_NAME}")
       string (REPLACE "_" "-" arg_TARGET "${arg_TARGET}")
    endif ()

    # The collection of include file paths is built by stitching the HEADER_DIR
    # on to the front of each of the INCLUDES.
    set (include_files "")
    foreach (include ${arg_INCLUDES})
       list (APPEND include_files "${arg_HEADER_DIR}/${include}")
    endforeach ()

    if (PSTORE_IS_INSIDE_LLVM)
        add_llvm_library (${arg_TARGET}
            STATIC
            ${arg_SOURCES}
            ${include_files}

            ADDITIONAL_HEADER_DIRS
            "${arg_HEADER_DIR}"
        )
        add_pstore_additional_compiler_flags (${arg_TARGET})

        # TODO: remove once we can upgrade to a C++14-enabled LLVM revision.
        if (CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
            target_compile_options (${arg_TARGET} PUBLIC "-std=c++${pstore_cxx_version}")
        endif ()
    else ()
        add_library (${arg_TARGET} STATIC ${arg_SOURCES} ${include_files})

        set_target_properties (${arg_TARGET} PROPERTIES
            CXX_STANDARD "${pstore_cxx_version}"
            CXX_STANDARD_REQUIRED Yes
            PUBLIC_HEADER "${arg_INCLUDES}"
        )

        pstore_enable_warnings (TARGET ${arg_TARGET})
        add_pstore_additional_compiler_flags (${arg_TARGET})

        target_include_directories (${arg_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    endif (PSTORE_IS_INSIDE_LLVM)

    install (
        TARGETS ${arg_TARGET}
        ARCHIVE DESTINATION lib/pstore
        PUBLIC_HEADER DESTINATION "include/pstore/${arg_NAME}"
        COMPONENT pstore
    )
    add_dependencies (install-pstore ${arg_TARGET})

    set_target_properties (${arg_TARGET} PROPERTIES FOLDER "pstore libraries")
    target_include_directories (${arg_TARGET} PUBLIC
        $<BUILD_INTERFACE:${PSTORE_ROOT_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
endfunction (add_pstore_library)


#######################
# add_pstore_executable
#######################

function (add_pstore_executable target)
    if (PSTORE_IS_INSIDE_LLVM)
        add_llvm_executable (${target} ${ARGN})
        add_pstore_additional_compiler_flags (${target})

        # TODO: remove once we can upgrade to a C++14-enabled LLVM revision.
        if (CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
            target_compile_options (${target} PUBLIC -std=c++14)
        endif ()
    else ()
        add_executable (${target} ${ARGN})
        set_target_properties (${target} PROPERTIES
            CXX_STANDARD "${pstore_cxx_version}"
            CXX_STANDARD_REQUIRED Yes
        )
        pstore_enable_warnings (TARGET ${target})
        add_pstore_additional_compiler_flags (${target})
    endif (PSTORE_IS_INSIDE_LLVM)

    set_target_properties (${target} PROPERTIES FOLDER "pstore executables")
    # On MSVC, link setargv.obj to support the two wildcards (? and *) on the command line.
    if (MSVC)
        set_target_properties (${target} PROPERTIES LINK_FLAGS "setargv.obj")
    endif ()
endfunction (add_pstore_executable)


#################
# add_pstore_tool
#################
# add_pstore_tool is a wrapper for add_pstore_executable which also creates an
# install target. Call this instead of add_pstore_executable to create a target
# for an executable that is to be installed.

function (add_pstore_tool name)
    add_pstore_executable (${name} ${ARGN})

    install (TARGETS ${name} RUNTIME DESTINATION bin COMPONENT pstore)
    add_dependencies (install-pstore ${name})
endfunction (add_pstore_tool)


####################
# add_pstore_example
####################

function (add_pstore_example name)
    add_pstore_executable (example-${name} ${ARGN})
    set_target_properties (example-${name} PROPERTIES
        EXCLUDE_FROM_ALL Yes
        FOLDER "pstore examples"
    )
    target_link_libraries (example-${name} PRIVATE pstore-core)
endfunction (add_pstore_example)


##########################
# add_pstore_test_library
##########################

function (add_pstore_test_library target_name)
    if (PSTORE_IS_INSIDE_LLVM)
        add_library (${target_name} STATIC ${ARGN})
        add_pstore_additional_compiler_flags (${target_name})

        # TODO: remove once we can upgrade to a C++14-enabled LLVM revision.
        if (CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
            target_compile_options (${target_name} PUBLIC -std=c++14)
        endif ()

        include_directories (${LLVM_MAIN_SRC_DIR}/utils/unittest/googletest/include)
        include_directories (${LLVM_MAIN_SRC_DIR}/utils/unittest/googlemock/include)
        target_link_libraries (${target_name} PUBLIC gtest)
    else ()
        add_library (${target_name} STATIC ${ARGN})
        set_target_properties (${target_name} PROPERTIES
            CXX_STANDARD "${pstore_cxx_version}"
            CXX_STANDARD_REQUIRED Yes
        )
        pstore_enable_warnings (TARGET ${target_name})
        add_pstore_additional_compiler_flags (${target_name})
        target_include_directories (${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        target_link_libraries (${target_name} PUBLIC gtest gmock)
    endif (PSTORE_IS_INSIDE_LLVM)

    set_target_properties (${target_name} PROPERTIES FOLDER "pstore test libraries")
endfunction (add_pstore_test_library)


######################
# add_pstore_unit_test
######################

function (add_pstore_unit_test target_name)
    if (PSTORE_IS_INSIDE_LLVM)
        add_unittest (PstoreUnitTests ${target_name} ${ARGN})
        add_pstore_additional_compiler_flags (${target_name})

        # TODO: remove once we can upgrade to a C++14-enabled LLVM revision.
        if (CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
            target_compile_options (${target_name} PUBLIC -std=c++14)
        endif ()
    else()
        add_executable (${target_name} ${ARGN})
        set_target_properties (${target_name} PROPERTIES
            FOLDER "pstore tests"
            CXX_STANDARD "${pstore_cxx_version}"
            CXX_STANDARD_REQUIRED Yes
        )
        target_include_directories (${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        pstore_enable_warnings (TARGET ${target_name} IS_UNIT_TEST)
        add_pstore_additional_compiler_flags (${target_name})
        target_link_libraries (${target_name} PRIVATE pstore-unit-test-harness gtest gmock)
    endif (PSTORE_IS_INSIDE_LLVM)
endfunction (add_pstore_unit_test)
