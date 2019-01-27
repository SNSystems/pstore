#*            _     _             _                  *
#*   __ _  __| | __| |  _ __  ___| |_ ___  _ __ ___  *
#*  / _` |/ _` |/ _` | | '_ \/ __| __/ _ \| '__/ _ \ *
#* | (_| | (_| | (_| | | |_) \__ \ || (_) | | |  __/ *
#*  \__,_|\__,_|\__,_| | .__/|___/\__\___/|_|  \___| *
#*                     |_|                           *
#===- cmake/add_pstore.cmake ----------------------------------------------===//
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

include (CheckCSourceCompiles)

macro(add_pstore_subdirectory name)
    if (PSTORE_IS_INSIDE_LLVM)
        add_llvm_subdirectory(PSTORE TOOL ${name})
    endif ()
endmacro()


# disable_clang_warning_if_possible
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Adds a switch to the compile options if supported by the compiler.
#
# target_name: The name of the target to which the compile option should be
#              added.
# flag:        The name of the compiler switch to be added.
# result_var:  The name of the CMake cache variable into which the result of the
#              test will be stored. This should be different for each switch
#              being tested.
function (disable_clang_warning_if_possible target_name flag result_var)
    set (PSTORE_OLD_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    set (CMAKE_REQUIRED_FLAGS "-Werror ${flag}")
    check_c_source_compiles ("int main () {}" ${result_var})
    set (CMAKE_REQUIRED_FLAGS "${PSTORE_OLD_CMAKE_REQUIRED_FLAGS}")
    if (${${result_var}})
        target_compile_options (${target_name} PRIVATE ${flag})
    endif ()
endfunction()


function (add_pstore_additional_compiler_flag name)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # For Clang, I enable all of the warnings and then disable some of
        # the unwanted ones.
        target_compile_options (${name} PRIVATE
            -Weverything
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-c99-extensions
            -Wno-deprecated
            -Wno-disabled-macro-expansion
            -Wno-exit-time-destructors
            -Wno-global-constructors
            -Wno-missing-noreturn
            -Wno-missing-variable-declarations
            -Wno-padded
            -Wno-shift-sign-overflow
            -Wno-undef
            -Wno-unused-macros
            -Wno-used-but-marked-unused
            -Wno-weak-vtables
        )

        disable_clang_warning_if_possible (${name}
            -Wno-nullability-extension
            PSTORE_CLANG_SUPPORTS_NX
        )
        disable_clang_warning_if_possible (${name}
            -Wno-zero-as-null-pointer-constant
            PSTORE_CLANG_SUPPORTS_NPC
        )
        # TODO: this warning is issued (at the time of writing) by gtest. It should only be disabled
        # for that target.
        disable_clang_warning_if_possible (${name}
            -Wno-inconsistent-missing-override
            PSTORE_CLANG_SUPPORTS_IMO
        )

        # Disable the "unused lambda capture" warning since we must capture extra variables for MSVC
        # to swallow the code. Remove when VS doesn't require capture.
        disable_clang_warning_if_possible (${name}
            -Wno-unused-lambda-capture
            PSTORE_CLANG_SUPPORTS_ULC
        )

        if (PSTORE_COVERAGE)
            target_compile_options (${name} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
            target_compile_options (${name} PRIVATE -fno-inline-functions)
            target_link_libraries (${name} PUBLIC -fprofile-instr-generate -fcoverage-mapping)
        endif (PSTORE_COVERAGE)

    elseif (CMAKE_COMPILER_IS_GNUCXX)

        target_compile_options (${name} PRIVATE
            -Wall
            -Wextra
            -pedantic
        )
        if (PSTORE_COVERAGE)
            target_compile_options (${name} PRIVATE -fprofile-arcs -ftest-coverage)
            target_link_libraries (${name} PUBLIC --coverage)
        endif (PSTORE_COVERAGE)

    elseif (MSVC)

        # 4127: conditional expression is constant. We're using C++11 therefore if constexpr is
        #       not available.
                # 4146: unary minus applied to unsigned, result still unsigned.
        # 4512: assignment operator could not be generated.
        target_compile_options (${name} PRIVATE
            -W4
            -wd4127
            -wd4146
            -wd4512
        )
        target_compile_definitions (${name} PRIVATE
            -D_CRT_SECURE_NO_WARNINGS
            -D_SCL_SECURE_NO_WARNINGS
        )

        if (PSTORE_COVERAGE)
            message (WARNING "PSTORE_COVERAGE is not yet implemented for MSVC")
        endif (PSTORE_COVERAGE)

    endif ()
endfunction(add_pstore_additional_compiler_flag)


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

function (add_pstore_library target_name)
    if (PSTORE_IS_INSIDE_LLVM)
        add_llvm_library (${target_name} STATIC ${ARGN})
    else ()
        add_library (${target_name} STATIC ${ARGN})

        set_property (TARGET ${target_name} PROPERTY CXX_STANDARD 11)
        set_property (TARGET ${target_name} PROPERTY CXX_STANDARD_REQUIRED Yes)
        add_pstore_additional_compiler_flag (${target_name})

        target_include_directories (${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

        # On Windows, we're a "Unicode" app.
        if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
            target_compile_definitions ("${target_name}" PRIVATE -DUNICODE -D_UNICODE)
        endif ()
    endif (PSTORE_IS_INSIDE_LLVM)

    set_target_properties (${target_name} PROPERTIES FOLDER "pstore libraries")
    target_include_directories (${target_name} PUBLIC
        $<BUILD_INTERFACE:${PSTORE_ROOT_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
endfunction(add_pstore_library)


#######################
# add_pstore_executable
#######################

function (add_pstore_executable target_name)
    if (PSTORE_IS_INSIDE_LLVM)
        add_llvm_executable ("${target_name}" ${ARGN})
    else ()
        add_executable ("${target_name}" ${ARGN})

        set_property (TARGET "${target_name}" PROPERTY CXX_STANDARD 11)
        set_property (TARGET "${target_name}" PROPERTY CXX_STANDARD_REQUIRED Yes)
        add_pstore_additional_compiler_flag (${target_name})

        # On Windows, we're a "Unicode" app.
        if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
            target_compile_definitions ("${target_name}" PRIVATE -DUNICODE -D_UNICODE)
        endif ()
    endif (PSTORE_IS_INSIDE_LLVM)

    set_target_properties ("${target_name}" PROPERTIES FOLDER "pstore executables")
    # On MSVC, link setargv.obj to support the two wildcards (? and *) on the command line.
    if (MSVC)
        set_target_properties ("${target_name}" PROPERTIES LINK_FLAGS "setargv.obj")
    endif ()
endfunction(add_pstore_executable)


####################
# add_pstore_example
####################

function (add_pstore_example name)
    add_pstore_executable (example-${name} ${ARGN})
    set_target_properties (example-${name} PROPERTIES EXCLUDE_FROM_ALL 1)
    set_target_properties (example-${name} PROPERTIES FOLDER "pstore examples")
    target_link_libraries (example-${name} PRIVATE pstore-core)
endfunction (add_pstore_example)


##########################
# add_pstore_test_library
##########################

function (add_pstore_test_library target_name)
    add_library (${target_name} STATIC ${ARGN})
    set_target_properties (${target_name} PROPERTIES FOLDER "pstore test libraries")
    set_property (TARGET ${target_name} PROPERTY CXX_STANDARD 11)
    set_property (TARGET ${target_name} PROPERTY CXX_STANDARD_REQUIRED Yes)

    target_include_directories (${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    add_pstore_additional_compiler_flag (${target_name})

    if (PSTORE_IS_INSIDE_LLVM)
        include_directories (${LLVM_MAIN_SRC_DIR}/utils/unittest/googletest/include)
        include_directories (${LLVM_MAIN_SRC_DIR}/utils/unittest/googlemock/include)
        target_link_libraries (${target_name} PUBLIC gtest)
    else ()
        target_link_libraries (${target_name} PUBLIC gtest gmock)
    endif (PSTORE_IS_INSIDE_LLVM)
endfunction(add_pstore_test_library)


######################
# add_pstore_unit_test
######################

function(add_pstore_unit_test test_dirname)
    if (PSTORE_IS_INSIDE_LLVM)
        add_unittest (PstoreUnitTests ${test_dirname} ${ARGN})
    else()
        add_executable (${test_dirname} ${ARGN})
        set_target_properties (${test_dirname} PROPERTIES FOLDER "pstore tests")
        set_property (TARGET ${test_dirname} PROPERTY CXX_STANDARD 11)
        set_property (TARGET ${test_dirname} PROPERTY CXX_STANDARD_REQUIRED Yes)

        target_include_directories (${test_dirname} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        add_pstore_additional_compiler_flag (${test_dirname})
        target_link_libraries (${test_dirname} PRIVATE pstore-unit-test-harness gtest gmock)
    endif(PSTORE_IS_INSIDE_LLVM)
endfunction(add_pstore_unit_test)
