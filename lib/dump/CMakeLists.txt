#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/dump/CMakeLists.txt ---------------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//

##########################
# pstore Dump Core Library
##########################

set (LLVM_LINK_COMPONENTS
    AllTargetsDescs
    AllTargetsDisassemblers
    AllTargetsInfos
    CodeGen
    DebugInfoDWARF
    DebugInfoPDB
    Demangle
    MC
    MCDisassembler
    Object
    Support
    Symbolize
)
add_pstore_library (
    TARGET pstore-dump-lib
    NAME dump
    SOURCES
        db_value.cpp
        error.cpp
        line_splitter.cpp
        mcdebugline_value.cpp
        mcdisassembler_value.cpp
        mcrepo_value.cpp
        value.cpp
    HEADER_DIR
        "${PSTORE_ROOT_DIR}/include/pstore/dump"
    INCLUDES
        db_value.hpp
        digest_opt.hpp
        error.hpp
        index_value.hpp
        line_splitter.hpp
        mcdebugline_value.hpp
        mcdisassembler_value.hpp
        mcrepo_value.hpp
        value.hpp
)
target_link_libraries (pstore-dump-lib PUBLIC pstore-adt pstore-core pstore-mcrepo)
