#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/mcrepo/CMakeLists.txt -------------------------------------------===//
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

set (pstore_mcrepo_public_include "${PSTORE_ROOT_DIR}/include/pstore/mcrepo")
add_pstore_library (
    NAME mcrepo
    SOURCES
        bss_section.cpp
        compilation.cpp
        debug_line_section.cpp
        dependents_section.cpp
        fragment.cpp
        generic_section.cpp
        repo_error.cpp
        section.cpp
    INCLUDES
        "${pstore_mcrepo_public_include}/bss_section.hpp"
        "${pstore_mcrepo_public_include}/compilation.hpp"
        "${pstore_mcrepo_public_include}/debug_line_section.hpp"
        "${pstore_mcrepo_public_include}/dependents_section.hpp"
        "${pstore_mcrepo_public_include}/fragment.hpp"
        "${pstore_mcrepo_public_include}/generic_section.hpp"
        "${pstore_mcrepo_public_include}/repo_error.hpp"
        "${pstore_mcrepo_public_include}/section.hpp"
        "${pstore_mcrepo_public_include}/sparse_array.hpp"
)
target_link_libraries (pstore-mcrepo PUBLIC pstore-core)
