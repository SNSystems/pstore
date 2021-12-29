#===- cmake/add_pstore.cmake ----------------------------------------------===//
#*            _     _             _                  *
#*   __ _  __| | __| |  _ __  ___| |_ ___  _ __ ___  *
#*  / _` |/ _` |/ _` | | '_ \/ __| __/ _ \| '__/ _ \ *
#* | (_| | (_| | (_| | | |_) \__ \ || (_) | | |  __/ *
#*  \__,_|\__,_|\__,_| | .__/|___/\__\___/|_|  \___| *
#*                     |_|                           *
#===----------------------------------------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//
include (CheckCSourceCompiles)
include (CheckCXXCompilerFlag)
include (GNUInstallDirs)

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


# pstore standalone compiler setup
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Sets the base warnings for a standalone build. This combines with the options
# set by pstore_add_additional_compiler_flags which is used in both standalone
# and "inside LLVM" builds.
function (pstore_standalone_compiler_setup)
    cmake_parse_arguments (arg # prefix
        "IS_UNIT_TEST" # options
        "TARGET" # one-value-keywords
        "" # multi-value-keywords
        ${ARGN}
    )

    # clang
    # ~~~~~
    set (clang_options )
    # Optimization flags.
    list (APPEND clang_options -mpopcnt)
    # FIXME: This option is not supported with AppleClang.
    #list (APPEND clang_options -fno-semantic-interposition)

    # Lots of warnings.
    list (APPEND clang_options
        -Weverything
        -Wno-c99-extensions
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-c++14-extensions
        -Wno-disabled-macro-expansion
        -Wno-exit-time-destructors
        -Wno-global-constructors
        -Wno-padded
        -Wno-weak-vtables
    )
    if (${arg_IS_UNIT_TEST})
        list (APPEND clang_options
            -Wno-unused-member-function
            -Wno-used-but-marked-unused
        )
    endif ()

    # GCC
    # ~~~
    set (gcc_options )
    # Optimization flags.
    list (APPEND gcc_options
        -fno-semantic-interposition
        -mpopcnt
    )
    # Lots of warnings.
    list (APPEND gcc_options
        -Wall
        -Wextra
        -pedantic
    )

    # Visual Studio
    # ~~~~~~~~~~~~~
    set (msvc_options )
    # Lots of warnings. Enable W4 but disable:
    # 4324: structure was padded due to alignment specifier
    list (APPEND msvc_options
        -W4
        -wd4324
    )

    if (PSTORE_WERROR)
        list (APPEND clang_options -Werror)
        list (APPEND gcc_options -Werror)
        list (APPEND msvc_options /WX)
    endif ()

    target_compile_options (${arg_TARGET} PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_options}>
        $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
        $<$<CXX_COMPILER_ID:MSVC>:${msvc_options}>
    )

endfunction (pstore_standalone_compiler_setup)


# pstore add additional compiler flags
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function (pstore_add_additional_compiler_flags target_name)
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

endfunction(pstore_add_additional_compiler_flags)


####################
# add pstore library
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
        pstore_add_additional_compiler_flags (${arg_TARGET})
    else ()
        add_library (${arg_TARGET} STATIC ${arg_SOURCES} ${include_files})

        set_target_properties (${arg_TARGET} PROPERTIES
            CXX_STANDARD "${pstore_cxx_version}"
            CXX_STANDARD_REQUIRED Yes
            CXX_EXTENSIONS Off
            # Full path of all of the public include files.
            PUBLIC_HEADER "${include_files}"
            # Produce PIC so that pstore static libraries can be used in a shared library.
            POSITION_INDEPENDENT_CODE Yes
        )

        pstore_standalone_compiler_setup (TARGET ${arg_TARGET})
        pstore_add_additional_compiler_flags (${arg_TARGET})

        target_include_directories (${arg_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    endif (PSTORE_IS_INSIDE_LLVM)

    install (
        TARGETS ${arg_TARGET}
        EXPORT pstore
        PUBLIC_HEADER
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/pstore/${arg_NAME}"
            COMPONENT pstore
        LIBRARY
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/pstore"
            COMPONENT pstore
        ARCHIVE
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/pstore"
            COMPONENT pstore
        INCLUDES
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )
    add_dependencies (install-pstore ${arg_TARGET})

    set_target_properties (${arg_TARGET} PROPERTIES FOLDER "pstore libraries")
    target_include_directories (${arg_TARGET} PUBLIC
        $<BUILD_INTERFACE:${PSTORE_ROOT_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDE_DIR}>
    )

endfunction (add_pstore_library)


#######################
# add_pstore_executable
#######################

function (add_pstore_executable target)
    if (PSTORE_IS_INSIDE_LLVM)
        add_llvm_executable (${target} ${ARGN})
        pstore_add_additional_compiler_flags (${target})
    else ()
        add_executable (${target} ${ARGN})
        set_target_properties (${target} PROPERTIES
            CXX_STANDARD "${pstore_cxx_version}"
            CXX_STANDARD_REQUIRED Yes
            CXX_EXTENSIONS Off
        )
        pstore_standalone_compiler_setup (TARGET ${target})
        pstore_add_additional_compiler_flags (${target})
    endif (PSTORE_IS_INSIDE_LLVM)

    set_target_properties (${target} PROPERTIES FOLDER "pstore executables")
    # On MSVC, link setargv.obj to support the two wildcards (? and *) on the command line.
    if (MSVC)
        set_target_properties (${target} PROPERTIES LINK_FLAGS "setargv.obj")
    endif ()
endfunction (add_pstore_executable)


#################
# add pstore tool
#################
# add_pstore_tool is a wrapper for add_pstore_executable which also creates an
# install target. Call this instead of add_pstore_executable to create a target
# for an executable that is to be installed.

function (add_pstore_tool name)
    add_pstore_executable (${name} ${ARGN})

    install (
        TARGETS ${name}
        EXPORT pstore
        RUNTIME
            DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT pstore
    )
    add_dependencies (install-pstore ${name})
endfunction (add_pstore_tool)


####################
# add pstore example
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
        pstore_add_additional_compiler_flags (${target_name})
        include_directories (${LLVM_MAIN_SRC_DIR}/utils/unittest/googletest/include)
        include_directories (${LLVM_MAIN_SRC_DIR}/utils/unittest/googlemock/include)
        target_link_libraries (${target_name} PUBLIC gtest)
    else ()
        add_library (${target_name} STATIC ${ARGN})
        set_target_properties (${target_name} PROPERTIES
            CXX_STANDARD "${pstore_cxx_version}"
            CXX_STANDARD_REQUIRED Yes
            CXX_EXTENSIONS Off
        )
        pstore_standalone_compiler_setup (TARGET ${target_name})
        pstore_add_additional_compiler_flags (${target_name})
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
        pstore_add_additional_compiler_flags (${target_name})
    else()
        add_executable (${target_name} ${ARGN})
        set_target_properties (${target_name} PROPERTIES
            FOLDER "pstore tests"
            CXX_STANDARD "${pstore_cxx_version}"
            CXX_STANDARD_REQUIRED Yes
            CXX_EXTENSIONS Off
        )
        target_include_directories (${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        pstore_standalone_compiler_setup (TARGET ${target_name} IS_UNIT_TEST)
        pstore_add_additional_compiler_flags (${target_name})
        target_link_libraries (${target_name} PRIVATE pstore-unit-test-harness gtest gmock)
    endif (PSTORE_IS_INSIDE_LLVM)
endfunction (add_pstore_unit_test)
