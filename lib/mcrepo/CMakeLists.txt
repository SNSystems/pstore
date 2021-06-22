#===- lib/mcrepo/CMakeLists.txt -------------------------------------------===//
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

add_pstore_library (
    NAME mcrepo
    SOURCES
        bss_section.cpp
        compilation.cpp
        debug_line_section.cpp
        linked_definitions_section.cpp
        fragment.cpp
        generic_section.cpp
        repo_error.cpp
        section.cpp
    HEADER_DIR
        "${PSTORE_ROOT_DIR}/include/pstore/mcrepo"
    INCLUDES
        bss_section.hpp
        compilation.hpp
        debug_line_section.hpp
        linked_definitions_section.hpp
        fragment.hpp
        generic_section.hpp
        repo_error.hpp
        section.hpp
)
target_link_libraries (pstore-mcrepo PUBLIC pstore-adt pstore-core)
