#*            _     _             _                  *
#*   __ _  __| | __| |  _ __  ___| |_ ___  _ __ ___  *
#*  / _` |/ _` |/ _` | | '_ \/ __| __/ _ \| '__/ _ \ *
#* | (_| | (_| | (_| | | |_) \__ \ || (_) | | |  __/ *
#*  \__,_|\__,_|\__,_| | .__/|___/\__\___/|_|  \___| *
#*                     |_|                           *
#===- CMakeModules/add_pstore.cmake ---------------------------------------===//
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

macro(add_pstore_subdirectory name)
    if (PSTORE_IS_INSIDE_LLVM)
        add_llvm_subdirectory(PSTORE TOOL ${name})
    endif ()
endmacro()


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

        target_compile_options     (${target_name} PRIVATE ${EXTRA_CXX_FLAGS})
        target_compile_definitions (${target_name} PRIVATE ${EXTRA_CXX_DEFINITIONS})

        target_include_directories (${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

        # for config.hpp
        target_include_directories (${target_name} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    endif (PSTORE_IS_INSIDE_LLVM)
    set_target_properties (${target_name} PROPERTIES FOLDER "pstore libraries")
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

        target_compile_options ("${target_name}" PRIVATE ${EXTRA_CXX_FLAGS})
        target_compile_definitions ("${target_name}" PRIVATE ${EXTRA_CXX_DEFINITIONS})

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
    target_link_libraries (example-${name} PRIVATE pstore-support-lib pstore)
endfunction (add_pstore_example)


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
        target_compile_options (${test_dirname} PRIVATE ${EXTRA_CXX_FLAGS})
        target_compile_definitions (${test_dirname} PRIVATE ${EXTRA_CXX_DEFINITIONS})
        target_link_libraries (${test_dirname} gmock_main gtest gmock)

        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # Disable warnings from the google test headers.
            target_compile_options (${test_dirname} PRIVATE
                -Wno-deprecated
                -Wno-inconsistent-missing-override
                -Wno-missing-variable-declarations
                -Wno-shift-sign-overflow
            )
        endif ()
    endif(PSTORE_IS_INSIDE_LLVM)

endfunction(add_pstore_unit_test)

# eof: CMakeModules/add_pstore.cmake
