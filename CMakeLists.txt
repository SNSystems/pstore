#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- CMakeLists.txt ------------------------------------------------------===//
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
##################################
#  ____  ____  _
# |  _ \/ ___|| |_ ___  _ __ ___
# | |_) \___ \| __/ _ \| '__/ _ \
# |  __/ ___) | || (_) | | |  __/
# |_|   |____/ \__\___/|_|  \___|
#
# Copyright (c) 2015-2016 by Sony Interactive Entertainment, Inc.

cmake_minimum_required (VERSION 3.1)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

# Set to YES (or NO) to enable (or disable) building with clang on Linux.
# TODO: allow this to be controlled from the outside world.
if (UNIX AND NOT APPLE)
  set (FORCE_CLANG NO)
endif ()

if (FORCE_AFL)
    message (STATUS "American Fuzzy Lop compilation enabled")
    if (FORCE_CLANG)
        set (AFL_C_NAME   afl-clang)
        set (AFL_CXX_NAME afl-clang++)
        set (CMAKE_CXX_COMPILER_ID "Clang")
    else ()
        set (AFL_C_NAME   afl-gcc)
        set (AFL_CXX_NAME afl-g++)
    endif ()

    message (STATUS "AFL_PATH=$ENV{AFL_PATH}")
    find_program (AFL_C_EXECUTABLE
                 NAMES ${AFL_C_NAME}
                 DOC "${AFL_C_NAME} location"
                 HINTS $ENV{AFL_PATH})
    find_program (AFL_CXX_EXECUTABLE
                 NAMES ${AFL_CXX_NAME}
                 DOC "${AFL_CXX_NAME} location"
                 HINTS $ENV{AFL_PATH})
    if (NOT AFL_C_EXECUTABLE)
       message (SEND_ERROR "** ${AFL_C_NAME} was not found")
    endif ()
    if (NOT AFL_CXX_EXECUTABLE)
        message (SEND_ERROR "** ${AFL_CXX_NAME} was not found")
    endif ()

    set (CMAKE_C_COMPILER      "${AFL_C_EXECUTABLE}")
    set (CMAKE_CXX_COMPILER    "${AFL_CXX_EXECUTABLE}")
    message (STATUS "American Fuzzy Lop: C Compiler is: ${CMAKE_C_COMPILER}")
    message (STATUS "American Fuzzy Lop: C++ Compiler is: ${CMAKE_CXX_COMPILER}")

elseif (FORCE_CLANG)

    message (STATUS "FORCE_CLANG is enabled")

    # If clang has been requested, then I need to find it and set the
    # C_COMPILER, CXX_COMPILER, and CXX_COMPILER_ID variables accordingly.
    # All of this needs to be done _before_ the project declaration.

    find_program (CLANG_EXECUTABLE   NAMES clang   DOC "clang location")
    find_program (CLANGXX_EXECUTABLE NAMES clang++ DOC "clang++ location")

    if (NOT (CLANG_EXECUTABLE AND CLANGXX_EXECUTABLE))
        message (SEND_ERROR "** clang was not found.")
    endif ()
    if (NOT CLANGXX_EXECUTABLE)
        message (SEND_ERROR "** clang++ was not found.")
    endif ()

    if (CLANG_EXECUTABLE AND CLANGXX_EXECUTABLE)
        message (STATUS "Found clang at: ${CLANG_EXECUTABLE}, clang++ at ${CLANGXX_EXECUTABLE}")
        set (CMAKE_C_COMPILER      "${CLANG_EXECUTABLE}")
        set (CMAKE_CXX_COMPILER    "${CLANGXX_EXECUTABLE}")
        set (CMAKE_CXX_COMPILER_ID "Clang")
    endif ()
endif()


# If we are not building as a part of LLVM, build pstore as a
# standalone project, using LLVM as an external library:
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project (pstore)
    set_property (GLOBAL PROPERTY USE_FOLDERS ON)
endif ()
set (PSTORE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")


if (FORCE_CLANG)
    set (CLANG_SANITIZER_FLAGS  "-fsanitize=undefined,address")
    #set (CLANG_SANITIZER_FLAGS  "-fsanitize=memory -fPIE -fno-omit-frame-pointer -fsanitize-memory-track-origins  -I/media/psf/Home/realwork/libcxx_msan/include -I/media/psf/Home/realwork/libcxx_msan/include/c++/v1 -Wno-reserved-id-macro -Wno-deprecated")
    set (CMAKE_EXE_LINKER_FLAGS_DEBUG  "${CLANG_SANITIZER_FLAGS}")
    #set (CMAKE_EXE_LINKER_FLAGS_DEBUG  "${CLANG_SANITIZER_FLAGS} -pie -stdlib=libc++ -L/media/psf/Home/realwork/libcxx_msan/lib -lc++abi -Wl,-rpath,/media/psf/Home/realwork/libcxx_msan/lib")


    set (CMAKE_C_FLAGS                "-Wall")
    set (CMAKE_C_FLAGS_DEBUG          "-O0 -g ${CLANG_SANITIZER_FLAGS}")
    set (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
    set (CMAKE_C_FLAGS_RELEASE        "-O4 -DNDEBUG")
    set (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

    set (CMAKE_CXX_FLAGS                "-Wall")
    set (CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_C_FLAGS_DEBUG}")
    set (CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_C_FLAGS_MINSIZEREL}")
    set (CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_C_FLAGS_RELEASE}")
    set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
endif ()

if (MSVC)
    # Microsoft's linker/archiver (not sure which) likes to warn when it sees
    # a library containing an object file with no code or data. I do it on
    # purpose (at the moment) so that the project contains the POSIX versions
    # of code. Disable the warning.

    set (CMAKE_STATIC_LINKER_FLAGS  "${CMAKE_STATIC_LINKER_FLAGS} /IGNORE:4221")
endif ()

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")


include_directories(BEFORE
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  )


# The "PSTORE_VENDOR_ID" is used in cases where pstore needs a system wide vendor-unique name
# (i.e. where two pstore-derived products shouldn't ever collide with one another).

if (NOT DEFINED PSTORE_VENDOR_ID)
    set (PSTORE_VENDOR_ID  "com.mycompany")
    message (WARNING  "PSTORE_VENDOR_ID not set: defaulting to \"${PSTORE_VENDOR_ID}\"")
endif ()


# Try to work out whether the pstore library is being built inside the
# larger LLVM framework or if it's a standalone build.

set (PSTORE_IS_INSIDE_LLVM No)
if (LLVM_MAIN_SRC_DIR)
    set (PSTORE_IS_INSIDE_LLVM Yes)
endif ()



# The pstore header and footer structures include crc32 values as an aid to validation:
# if the CRC value doesn't match then the structure is not valid. However, is the CRC
# does match then this is not an absolute guarantee that the remaining fields can be
# trusted. For this reason. it's possible to build with the CRC checks disabled to make
# finding bugs in the validity checks quicker and easier.
option (PSTORE_CRC_CHECKS_ENABLED "Perform CRC checks on pstore header and footer records. Disable for fuzzing." Yes)

option (PSTORE_POSIX_SMALL_FILES "On POSIX systems, keep pstore files as small as possible")
option (PSTORE_ALWAYS_SPANNING "A debugging aid which forces all requests to behave as 'spanning' pointers")


#
# If we're using the a fuzzing library (e.g. American Fuzzy Lop)
# disable the library's CRC validation checks and create the
# smallest files that we can to reduce the wasted work that the
# tool will need to perform.
#
if (FORCE_AFL)
    set (PSTORE_CRC_CHECKS_ENABLED NO)
    set (PSTORE_POSIX_SMALL_FILES  YES)
endif ()

if (NOT PSTORE_CRC_CHECKS_ENABLED)
    message (WARNING "CRC checks are disabled")
endif ()


set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DGSL_TERMINATE_ON_CONTRACT_VIOLATION")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DGSL_UNENFORCED_ON_CONTRACT_VIOLATION")


#
# Define a pair of variables ("EXTRA_CXX_FLAGS" and
# "EXTRA_CXX_DEFINITIONS" which a project that wants the standard set
# of PStore compiler flags should apply using the cmake
# target_compile_options() and target_compile_definitions()
# functions. This avoids cranking up the warning level on the external
# projects that we're using and getting a big pile of unwanted
# warnings as a result.
#

set (EXTRA_CXX_FLAGS "")
set (EXTRA_CXX_DEFINITIONS "")

if (NOT PSTORE_IS_INSIDE_LLVM)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")

        # For Clang, I enable all of the warnings and then disable some of
        # the unwanted ones.
        set (EXTRA_CXX_FLAGS
            -Weverything
            -Wno-undef
            -Wno-padded
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-c99-extensions
            -Wno-weak-vtables
            -Wno-missing-noreturn
            -Wno-global-constructors
            -Wno-exit-time-destructors
            -Wno-used-but-marked-unused
            -Wno-unused-macros)

    elseif (CMAKE_COMPILER_IS_GNUCXX)

        set (EXTRA_CXX_FLAGS -Wall -Wextra -pedantic)

    elseif (MSVC)

        set (EXTRA_CXX_FLAGS "/W4")

        # Disable the "assignment operator could not be generated"
        # message. It's pointless since we'll see an error if an
        # assignment operator is needed.
        LIST (APPEND EXTRA_CXX_FLAGS /wd4512)

        set (EXTRA_CXX_DEFINITIONS -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)

    endif ()
endif (NOT PSTORE_IS_INSIDE_LLVM)

if (NOT DEFINED PSTORE_SERIALIZE_EXAMPLES)
   set (PSTORE_SERIALIZE_EXAMPLES  NO)
endif ()



if (PSTORE_IS_INSIDE_LLVM)
    add_custom_target (PstoreUnitTests)
    set_target_properties(PstoreUnitTests PROPERTIES FOLDER "pstore tests")
endif ()

add_subdirectory (3rd_party)
add_subdirectory (lib)    # add the PStore library itself
add_subdirectory (examples)
add_subdirectory (tools) # Add the utility tools
add_subdirectory (unittests) # Add the unit tests


##############
# The LIT configuration files
#
# We write one lit configuration file for each of the configurations
# that are being created for the build system (1 in the case of a makefile,
# more for Xcode or Visual Studio: conventionally Debug, Release,
# MinSizeRel, and RelWithDebInfo). Each one of these is used to
# configure the tests for that particular set of binaries.

function (write_lit_config LIT_CONFIG_DIR)
    set (CONFIG_DIR "${LIT_CONFIG_DIR}")
    message (STATUS "Writing LIT configuration to ${LIT_CONFIG_DIR}")
    configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.in"
                    "${LIT_CONFIG_DIR}/lit.cfg"
                    @ONLY)
endfunction (write_lit_config)


list (LENGTH CMAKE_CONFIGURATION_TYPES NUM_CONFIGURATIONS)
if (${NUM_CONFIGURATIONS})
    foreach (config ${CMAKE_CONFIGURATION_TYPES})
        if (NOT PSTORE_IS_INSIDE_LLVM)
            write_lit_config ("${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${config}")
        else ()
            write_lit_config ("${CMAKE_BINARY_DIR}/${config}/bin")
        endif()
    endforeach (config)
else ()
    write_lit_config ("${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif ()

# On Windows hosts, provide an option to specify the path to the GnuWin32 tools.
if (WIN32 AND NOT CYGWIN)
    set (LIT_TOOLS_DIR "" CACHE PATH "Path to GnuWin32 tools")
endif ()


##############
# Add a custom target for running the system tests.

if (PSTORE_IS_INSIDE_LLVM)
    set (LIT_EXECUTABLE "${LLVM_MAIN_SRC_DIR}")
else ()
    set (LIT_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}")
endif (PSTORE_IS_INSIDE_LLVM)
set (LIT_EXECUTABLE "${LIT_EXECUTABLE}/utils/lit/lit.py")

# On Windows, I have to explicitly run python to ensure that lit.py is treated as
# a python program rather than something to be opened with a random text editor.
find_program (PYTHON_EXECUTABLE
    NAMES python
    DOC "python executbale location"
)
set (LIT_PARAMS "")
# Add the examples to the collection of tests.
if (PSTORE_EXAMPLES)
   list (APPEND LIT_PARAMS --param examples)
endif ()

# Add a target to the build which will run the system tests.
add_custom_target (pstore-system-tests
    COMMAND "${PYTHON_EXECUTABLE}" "${LIT_EXECUTABLE}" ${LIT_PARAMS} -s -v "$<TARGET_FILE_DIR:pstore-write>"
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Running system tests with LIT"
    VERBATIM
)
set_target_properties (pstore-system-tests PROPERTIES FOLDER "pstore tests")
add_dependencies (pstore-system-tests
    pstore-broker-poker
    pstore-brokerd
    pstore-dump
    pstore-hamt-test
    pstore-mangle
    pstore-read
    pstore-sieve
    pstore-vacuumd
    pstore-write
)
if (PSTORE_EXAMPLES)
   add_dependencies (pstore-system-tests pstore-examples)
endif ()



######
# Add a target to generate API documentation with Doxygen

find_package (Doxygen)
if (DOXYGEN_FOUND)
    if (DOXYGEN_DOT_FOUND)
        message ("-- Found dot at: ${DOXYGEN_DOT_EXECUTABLE}")

        get_filename_component (DOXYGEN_DOT_DIRECTORY
                                "${DOXYGEN_DOT_EXECUTABLE}"
                                DIRECTORY)
        message (STATUS "Dot path is: ${DOXYGEN_DOT_DIRECTORY}")
    else ()
        message (WARNING "dot was not found.")
    endif (DOXYGEN_DOT_FOUND)

    if (WIN32)
        set (DOXYGEN_PREDEFINED "_WIN32")
    else ()
        set (DOXYGEN_PREDEFINED "")
    endif (WIN32)


    message (STATUS "Copying Doxyfile.in to '${CMAKE_CURRENT_BINARY_DIR}/Doxyfile'")
    configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in"
                    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
                    @ONLY)
    add_custom_target (doc
                       ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                       COMMENT "Generating API documentation with Doxygen" VERBATIM)
else ()
    message (WARNING "Doxygen was not found.")
endif (DOXYGEN_FOUND)

# eof: CMakeLists.txt
