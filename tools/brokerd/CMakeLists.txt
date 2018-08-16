#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- tools/brokerd/CMakeLists.txt ----------------------------------------===//
# Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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

include (add_pstore)

###########################
# pstore-brokerd Executable
###########################

if (PSTORE_IS_INSIDE_LLVM)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
endif ()
add_pstore_executable (pstore-brokerd
    main.cpp
    switches.hpp
    switches.cpp
)
if (PSTORE_IS_INSIDE_LLVM)
    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
endif ()

if (PSTORE_IS_INSIDE_LLVM)
target_link_libraries (pstore-brokerd pstore-cmd-util-ex pstore-broker)
else ()
target_link_libraries (pstore-brokerd pstore-cmd-util pstore-broker)
endif ()

run_pstore_unit_test (pstore-brokerd pstore-broker-unit-tests)


#######################
# pstore_broker_service
#######################
if (WIN32)
    add_subdirectory (service)
endif ()


###################
# clang-tidy-broker
###################
include (clang-tidy)
if (CLANG_TIDY_FOUND)
    set (CLANG_TIDY_CHECKS -checks=cert-*,misc-*,modernize-*,--cppcoreguidelines-*,clang-analyzer-nullability*)

    # Collect the *.cpp files: clang-tidy will scan these.
    get_target_property (SOURCE_FILES pstore-broker SOURCES)
    list (FILTER SOURCE_FILES INCLUDE REGEX "cpp")

    # Tell clang-tidy about the target's include directories.
    set (TARGET_INCLUDE "")
    get_target_property (BRROKER_LIB_INCLUDES pstore-broker INCLUDE_DIRECTORIES)
    foreach (P ${BROKER_LIB_INCLUDES})
        list (APPEND TARGET_INCLUDE "-I" "${P}")
    endforeach (P)

    # Add in the pstore public includes
    get_target_property (PSTORE_LIB_INCLUDES pstore-core INTERFACE_INCLUDE_DIRECTORIES)
    foreach (P ${PSTORE_LIB_INCLUDES})
        list (APPEND TARGET_INCLUDE "-I" "${P}")
    endforeach (P)

    add_custom_target(
        "clang-tidy-broker"
        COMMAND "${CLANG_TIDY}"
        ${CLANG_TIDY_CHECKS}
        ${SOURCE_FILES}
        --
        -std=c++14
        ${TARGET_INCLUDE}
        ${CLANG_TIDY_SYS_INCLUDE}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
else()

endif()

