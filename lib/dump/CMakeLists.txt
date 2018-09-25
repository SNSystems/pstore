#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/dump/CMakeLists.txt ---------------------------------------------===//
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

##########################
# PStore Dump Core Library
##########################

set (PSTORE_DUMP_INCLUDE_DIR "${PSTORE_ROOT_DIR}/include/pstore/dump")
set (PSTORE_DUMP_LIB_SRC
    "${PSTORE_DUMP_INCLUDE_DIR}/db_value.hpp"
    "${PSTORE_DUMP_INCLUDE_DIR}/error.hpp"
    "${PSTORE_DUMP_INCLUDE_DIR}/index_value.hpp"
    "${PSTORE_DUMP_INCLUDE_DIR}/line_splitter.hpp"
    "${PSTORE_DUMP_INCLUDE_DIR}/mcdebugline_value.hpp"
    "${PSTORE_DUMP_INCLUDE_DIR}/mcdisassembler_value.hpp"
    "${PSTORE_DUMP_INCLUDE_DIR}/mcrepo_value.hpp"
    "${PSTORE_DUMP_INCLUDE_DIR}/value.hpp"

    db_value.cpp
    error.cpp
    line_splitter.cpp
    mcdebugline_value.cpp
    mcdisassembler_value.cpp
    mcrepo_value.cpp
    value.cpp
)

include (add_pstore)
set(LLVM_LINK_COMPONENTS
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
add_pstore_library (pstore-dump-lib ${PSTORE_DUMP_LIB_SRC})
target_link_libraries (pstore-dump-lib PUBLIC pstore-core pstore-mcrepo)
if (PSTORE_IS_INSIDE_LLVM)
    target_link_libraries (pstore-dump-lib PUBLIC LLVMSupport)
endif (PSTORE_IS_INSIDE_LLVM)

