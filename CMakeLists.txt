#===- CMakeLists.txt ------------------------------------------------------===//
#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===----------------------------------------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//

cmake_minimum_required (VERSION 3.4)

include (CheckCSourceCompiles)
include (CMakeDependentOption)
include (CPack)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


# Work out whether the pstore library is being built inside the
# larger LLVM framework or if it's a standalone build.
set (PSTORE_IS_INSIDE_LLVM No)
if (LLVM_MAIN_SRC_DIR)
    set (PSTORE_IS_INSIDE_LLVM Yes)
endif ()

cmake_dependent_option (PSTORE_EXCEPTIONS
    "Compile with exceptions enabled (always Off when compiling inside LLVM)"
    On
    "NOT PSTORE_IS_INSIDE_LLVM" Off
)
message (STATUS "Exception handling is ${PSTORE_EXCEPTIONS}")

# If we are not building as a part of LLVM, build pstore as a
# standalone project, using LLVM as an external library:
if (NOT PSTORE_IS_INSIDE_LLVM)
    project (pstore)
    set_property (GLOBAL PROPERTY USE_FOLDERS ON)
endif ()
set (PSTORE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Define pstore source and binary dirs similar to the definitions in clang
set (PSTORE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (PSTORE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

if (MSVC)
    # Microsoft's linker/archiver (not sure which) likes to warn when it sees
    # a library containing an object file with no code or data. I do it on
    # purpose (at the moment) so that the project contains the POSIX versions
    # of code. Disable the warning.
    set (CMAKE_STATIC_LINKER_FLAGS  "${CMAKE_STATIC_LINKER_FLAGS} /IGNORE:4221")
endif ()

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set (PSTORE_CONFIG_DIR "${CMAKE_BINARY_DIR}/include")
include_directories(BEFORE "${PSTORE_CONFIG_DIR}")


# The "PSTORE_VENDOR_ID" is used in cases where pstore needs a system wide vendor-unique name
# (i.e. where two pstore-derived products shouldn't ever collide with one another).

if (NOT DEFINED PSTORE_VENDOR_ID)
    set (PSTORE_VENDOR_ID  "com.mycompany")
    message (STATUS  "PSTORE_VENDOR_ID not set: defaulting to \"${PSTORE_VENDOR_ID}\"")
endif ()



# The pstore header and footer structures include crc32 values as an aid to validation:
# if the CRC value doesn't match then the structure is not valid. However, if the CRC
# does match then this is not an absolute guarantee that the remaining fields can be
# trusted. For this reason, it's possible to build with the CRC checks disabled to make
# it quicker and easiesr for a fuzzer to find bugs in the library.
option (PSTORE_CRC_CHECKS_ENABLED "Perform CRC checks on pstore header and footer records. Disable for fuzzing." Yes)
option (PSTORE_SIGNATURE_CHECKS_ENABLED "Perform internal structure signature checks. Disable for fuzzing." Yes)

option (PSTORE_POSIX_SMALL_FILES "On POSIX systems, keep pstore files as small as possible")
option (PSTORE_ALWAYS_SPANNING "A debugging aid which forces all requests to behave as 'spanning' pointers")

# TODO: PSTORE_ENABLE_BROKER is only implemented to enable testing with the early prepo compiler that doesn't yet support exceptions.
option (PSTORE_ENABLE_BROKER "Build broker related libraries and tools and run broker system tests. Disable if the compiler does not support exceptions." Yes)
option (PSTORE_VALGRIND "Run unit tests using Valgrind (if available).")
option (PSTORE_COVERAGE "Enable generation of coverage reports.")
option (PSTORE_DISABLE_UINT128_T "Disable support for __uint128_t")
option (PSTORE_CLANG_TIDY_ENABLED "Enable generation of clang-tidy targets")
option (PSTORE_NOISY_UNIT_TESTS "Produce complete ('noisy') output from the unit test executables")
option (PSTORE_WERROR "Compiler warnings are errors")

# The name of the vacuum (GC) executable.
set (PSTORE_VACUUM_TOOL_NAME "pstore-vacuumd")


if (NOT PSTORE_CRC_CHECKS_ENABLED)
    message (WARNING "CRC checks are disabled")
endif ()
if (NOT PSTORE_SIGNATURE_CHECKS_ENABLED)
    message (WARNING "Signature checks are disabled")
endif ()


find_program (PSTORE_KLEE_PATH klee)
set (PSTORE_BITCODE No)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang$")
    if ("${PSTORE_KLEE_PATH}" STREQUAL "PSTORE_KLEE_PATH-NOTFOUND")
        message (STATUS "Klee was not found")
    else ()
        message (STATUS "Found klee at: ${PSTORE_KLEE_PATH}")
        set (PSTORE_BITCODE Yes)
    endif ()
endif ()


if (NOT PSTORE_IS_INSIDE_LLVM)
    # Some basic tweaks to the compiler switches.
    if (MSVC)
        # VS 2017 : Disable warnings from from gtest code, using deprecated code related to TR1
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING")
        set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING")
        message (STATUS "Add flag to disable VS2017 warnings from gtest -  _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING")
    elseif (CMAKE_HOST_SYSTEM_NAME MATCHES "SunOS")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
        message (STATUS "Add flag to enable 64-bit build")
    endif ()
endif (NOT PSTORE_IS_INSIDE_LLVM)

if (NOT DEFINED PSTORE_SERIALIZE_EXAMPLES)
   set (PSTORE_SERIALIZE_EXAMPLES  NO)
endif ()



if (PSTORE_IS_INSIDE_LLVM)
    add_custom_target (PstoreUnitTests)
    set_target_properties(PstoreUnitTests PROPERTIES FOLDER "pstore tests")
endif (PSTORE_IS_INSIDE_LLVM)



# A custom target from which the installs will hang.
add_custom_target (install-pstore
    COMMAND "${CMAKE_COMMAND}"
            -D CMAKE_INSTALL_COMPONENT=pstore
            -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    USES_TERMINAL
)


add_subdirectory (3rd_party)

include (clang_tidy)
pstore_add_tidy_all_target ()
include (klee)
pstore_add_klee_run_all_target (PSTORE_BITCODE)

include (run_pstore_unit_test)
add_subdirectory (lib)       # Add the pstore libraries
add_subdirectory (examples)
add_subdirectory (tools)     # Add the utility tools
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





##############
# Add a custom target for running the system tests.

if (PSTORE_IS_INSIDE_LLVM)
    set (LIT_EXECUTABLE "${LLVM_MAIN_SRC_DIR}/utils/lit/lit.py")
else ()
    set (LIT_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/lit/lit.py")
endif (PSTORE_IS_INSIDE_LLVM)


# On Windows, I have to explicitly run python to ensure that lit.py is treated as
# a python program rather than something to be opened with a random text editor.
find_program (PYTHON_EXECUTABLE
    NAMES python
    DOC "python executable location"
)
set (LIT_PARAMS "")
# Add the examples to the collection of tests.
if (PSTORE_EXAMPLES)
    list (APPEND LIT_PARAMS --param examples)
endif ()
# Add the broker system tests to the collection of tests.
if (PSTORE_ENABLE_BROKER)
    list (APPEND LIT_PARAMS --param broker)
endif ()
if (PSTORE_IS_INSIDE_LLVM)
    list (APPEND LIT_PARAMS --param is_inside_llvm)
endif ()


# One or more tests uses Node.js/npm so let's check that it's available and work out which
# version is installed.
include (nodejs)
pstore_find_nodejs (nodejs_executable node_version_major node_version_minor node_version_patch)
if (nodejs_executable)
    if (node_version_major GREATER_EQUAL 8)
        message (STATUS "Enabling Node.js in system tests")
        list (APPEND LIT_PARAMS --param "nodejs=${nodejs_executable}")
    else ()
        message (STATUS "Version 8 or later is required for system tests")
    endif ()
endif ()

pstore_find_npm (npm_executable npm_version_major npm_version_minor npm_version_patch)
if (npm_executable)
    message (STATUS "Enabling NPM in system tests")
    list (APPEND LIT_PARAMS --param "npm=${npm_executable}")
endif (npm_executable)



# Add a target to the build which will run the system tests.
add_custom_target (pstore-system-tests
    COMMAND "${PYTHON_EXECUTABLE}"
        "${LIT_EXECUTABLE}"
        ${LIT_PARAMS}
        --echo-all-commands
        "$<TARGET_FILE_DIR:pstore-write>"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running system tests with LIT"
    VERBATIM
)
set_target_properties (pstore-system-tests PROPERTIES FOLDER "pstore tests")

if (PSTORE_IS_INSIDE_LLVM)
    add_dependencies (pstore-system-tests
        FileCheck
        llc
        opt
    )
endif ()

add_dependencies (pstore-system-tests
    pstore-broker-poker
    pstore-dump
    pstore-export
    pstore-hamt-test
    pstore-import
    pstore-lock-test
    pstore-mangle
    pstore-read
    pstore-sieve
    pstore-vacuumd
    pstore-write
)
if (PSTORE_ENABLE_BROKER)
    add_dependencies (pstore-system-tests pstore-brokerd)
endif (PSTORE_ENABLE_BROKER)



if (PSTORE_EXAMPLES)
   set_target_properties (pstore-examples PROPERTIES FOLDER "pstore examples")
   set_target_properties (pstore-examples-serialize PROPERTIES FOLDER "pstore examples")
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
        message (STATUS "dot was not found.")
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
endif (DOXYGEN_FOUND)

include (GNUInstallDirs)
install (
    EXPORT pstore
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pstore"
    NAMESPACE pstore::
)

# List of four values that specify what project to install. The four values are:
# - Build directory
# - Project Name
# - Project Component
# - Directory.
set (CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR};${CMAKE_PROJECT_NAME};ALL;/")
