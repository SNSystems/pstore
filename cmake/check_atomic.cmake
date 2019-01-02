#*       _               _            _                  _       *
#*   ___| |__   ___  ___| | __   __ _| |_ ___  _ __ ___ (_) ___  *
#*  / __| '_ \ / _ \/ __| |/ /  / _` | __/ _ \| '_ ` _ \| |/ __| *
#* | (__| | | |  __/ (__|   <  | (_| | || (_) | | | | | | | (__  *
#*  \___|_| |_|\___|\___|_|\_\  \__,_|\__\___/|_| |_| |_|_|\___| *
#*                                                               *
#===- cmake/check_atomic.cmake --------------------------------------------===//
# Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
INCLUDE(CheckCXXSourceCompiles)

# Sometimes linking against libatomic is required for atomic ops, if
# the platform doesn't support lock-free atomics.

# We modified LLVM's CheckAtomic module and have it check for 64-bit
# atomics.

function(pstore_check_cxx_atomics varname)
  set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS "-std=c++11 ${CMAKE_REQUIRED_FLAGS}")
  check_cxx_source_compiles("
#include <cstdint>
#include <atomic>
std::atomic<std::uint64_t> x (0);

int main() {
  return x.is_lock_free();
}
" ${varname})
  set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
endfunction(pstore_check_cxx_atomics)

pstore_check_cxx_atomics(PSTORE_HAVE_CXX_ATOMICS_WITHOUT_LIB)

# If not, check if the atomic library works with it.
if(NOT PSTORE_HAVE_CXX_ATOMICS_WITHOUT_LIB)
  list(APPEND CMAKE_REQUIRED_LIBRARIES "atomic")
  pstore_check_cxx_atomics(PSTORE_HAVE_CXX_ATOMICS_WITH_LIB)
  if (NOT PSTORE_HAVE_CXX_ATOMICS_WITH_LIB)
    message(WARNING "Host compiler must support std::atomic!")
  endif()
endif()
