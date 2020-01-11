#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/dump/CMakeLists.txt ---------------------------------------------===//
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

##########################
# PStore Dump Core Library
##########################

set (pstore_dump_include_dir "${PSTORE_ROOT_DIR}/include/pstore/dump")
set (LLVM_LINK_COMPONENTS
    AllTargetsAsmPrinters
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
        digest_opt.cpp
        error.cpp
        line_splitter.cpp
        mcdebugline_value.cpp
        mcdisassembler_value.cpp
        mcrepo_value.cpp
        value.cpp
    INCLUDES
        "${pstore_dump_include_dir}/db_value.hpp"
        "${pstore_dump_include_dir}/digest_opt.hpp"
        "${pstore_dump_include_dir}/error.hpp"
        "${pstore_dump_include_dir}/index_value.hpp"
        "${pstore_dump_include_dir}/line_splitter.hpp"
        "${pstore_dump_include_dir}/mcdebugline_value.hpp"
        "${pstore_dump_include_dir}/mcdisassembler_value.hpp"
        "${pstore_dump_include_dir}/mcrepo_value.hpp"
        "${pstore_dump_include_dir}/value.hpp"
)
target_link_libraries (pstore-dump-lib PUBLIC pstore-core pstore-mcrepo)
