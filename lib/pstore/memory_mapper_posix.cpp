//*                                             *
//*  _ __ ___   ___ _ __ ___   ___  _ __ _   _  *
//* | '_ ` _ \ / _ \ '_ ` _ \ / _ \| '__| | | | *
//* | | | | | |  __/ | | | | | (_) | |  | |_| | *
//* |_| |_| |_|\___|_| |_| |_|\___/|_|   \__, | *
//*                                      |___/  *
//*                                         *
//*  _ __ ___   __ _ _ __  _ __   ___ _ __  *
//* | '_ ` _ \ / _` | '_ \| '_ \ / _ \ '__| *
//* | | | | | | (_| | |_) | |_) |  __/ |    *
//* |_| |_| |_|\__,_| .__/| .__/ \___|_|    *
//*                 |_|   |_|               *
//===- lib/pstore/memory_mapper_posix.cpp ---------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc. 
// All rights reserved.
// 
// Developed by: 
//   Toolchain Team 
//   SN Systems, Ltd. 
//   www.snsystems.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
// 
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
// 
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
//===----------------------------------------------------------------------===//
/// \file memory_mapper_posix.cpp
/// \brief POSIX implementation of the platform-independent memory-mapped file

#include "pstore/memory_mapper.hpp"

#if !defined(_WIN32)

// Standard headers
#include <cassert>
#include <cerrno>
#include <limits>
#include <sstream>

// Platform headers
#include <sys/mman.h>
#include <unistd.h>

#include "pstore_support/error.hpp"

namespace pstore {

    //*                 _                                                _           *
    //*   ___ _   _ ___| |_ ___ _ __ ___    _ __   __ _  __ _  ___   ___(_)_______   *
    //*  / __| | | / __| __/ _ \ '_ ` _ \  | '_ \ / _` |/ _` |/ _ \ / __| |_  / _ \  *
    //*  \__ \ |_| \__ \ ||  __/ | | | | | | |_) | (_| | (_| |  __/ \__ \ |/ /  __/  *
    //*  |___/\__, |___/\__\___|_| |_| |_| | .__/ \__,_|\__, |\___| |___/_/___\___|  *
    //*       |___/                        |_|          |___/                        *
    // sysconf [static]
    // ~~~~~~~
    unsigned system_page_size::sysconf () {
        long result = ::sysconf (_SC_PAGESIZE);
        if (result == -1) {
            raise (errno_erc {errno}, "sysconf(_SC_PAGESIZE)");
        }
        assert (result > 0 && result <= std::numeric_limits<unsigned>::max ());
        return static_cast<unsigned> (result);
    }

} // namespace pstore


namespace pstore {

    // read_only
    // ~~~~~~~~~
    void memory_mapper_base::read_only_impl (void * addr, std::size_t len) {
        if (::mprotect (addr, len, PROT_READ) == -1) {
            raise (errno_erc {errno}, "mprotect");
        }
    }

} // namespace pstore


namespace pstore {

    //*   _ __ ___   ___ _ __ ___   ___  _ __ _   _    _ __ ___   __ _ _ __  _ __   ___ _ __   *
    //*  | '_ ` _ \ / _ \ '_ ` _ \ / _ \| '__| | | |  | '_ ` _ \ / _` | '_ \| '_ \ / _ \ '__|  *
    //*  | | | | | |  __/ | | | | | (_) | |  | |_| |  | | | | | | (_| | |_) | |_) |  __/ |     *
    //*  |_| |_| |_|\___|_| |_| |_|\___/|_|   \__, |  |_| |_| |_|\__,_| .__/| .__/ \___|_|     *
    //*                                       |___/                   |_|   |_|                *
    // (ctor)
    // ~~~~~~
    memory_mapper::memory_mapper (file::file_handle & file, bool write_enabled,
                                  std::uint64_t offset, std::uint64_t length)
            : memory_mapper_base (mmap (file, write_enabled, offset, length), write_enabled, offset,
                                  length) {}

    // mmap
    // ~~~~
    std::shared_ptr<void> memory_mapper::mmap (file::file_handle & file, bool write_enabled,
                                               std::uint64_t offset, std::uint64_t length) {

        assert (offset % system_page_size ().get () == 0);
        // TODO: should this function throw here rather than assert?
        assert (offset < std::numeric_limits<off_t>::max ());
        void * ptr = ::mmap (nullptr, // base address
                             length,
                             PROT_READ | (write_enabled ? PROT_WRITE : 0), // protection flags
                             MAP_SHARED, file.raw_handle (), static_cast<off_t> (offset));
        void * map_failed = MAP_FAILED; // NOLINT
        if (ptr == map_failed) {
            int const last_error = errno;
            std::ostringstream message;
            message << R"(Could not memory map file )" << file.path () << '\"';
            raise (errno_erc {last_error}, message.str ());
        }

        auto deleter = [length](void * p) {
            if (::munmap (p, length) == -1) {
                raise (errno_erc {errno}, "munmap");
            }
        };
        return std::shared_ptr<void> (ptr, deleter);
    }

} // namespace pstore
#endif //! defined (_WIN32)
// eof: lib/pstore/memory_mapper_posix.cpp
