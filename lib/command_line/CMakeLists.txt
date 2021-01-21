#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/command_line/CMakeLists.txt -------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#===----------------------------------------------------------------------===//
include (add_pstore)

set (pstore_command_line_include_dir "${PSTORE_ROOT_DIR}/include/pstore/command_line")
set (pstore_command_line_src
    category.cpp
    command_line.cpp
    csv.cpp
    help.cpp
    modifiers.cpp
    option.cpp
    parser.cpp
    revision_opt.cpp
    str_to_revision.cpp
    string_distance.cpp
    tchar.cpp
    word_wrapper.cpp
)
set (pstore_command_line_includes
    category.hpp
    command_line.hpp
    csv.hpp
    help.hpp
    modifiers.hpp
    option.hpp
    parser.hpp
    revision_opt.hpp
    str_to_revision.hpp
    stream_traits.hpp
    string_distance.hpp
    tchar.hpp
    word_wrapper.hpp
)

add_pstore_library (TARGET pstore-command-line
    NAME command_line
    SOURCES "${pstore_command_line_src}"
    HEADER_DIR "${pstore_command_line_include_dir}"
    INCLUDES "${pstore_command_line_includes}"
)
target_link_libraries (pstore-command-line PUBLIC pstore-adt pstore-os pstore-support)
add_clang_tidy_target (pstore-command-line)

if (NOT PSTORE_EXCEPTIONS)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)

    add_pstore_library (TARGET pstore-command-line-ex
        NAME pstore_command_line_ex
        SOURCES "${pstore_command_line_src}"
        HEADER_DIR "${pstore_command_line_include_dir}"
        INCLUDES "${pstore_command_line_includes}"
    )

    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
    set (PSTORE_EXCEPTIONS No)

    target_link_libraries (pstore-command-line-ex PUBLIC pstore-adt-ex pstore-os-ex pstore-support-ex)
endif (NOT PSTORE_EXCEPTIONS)
