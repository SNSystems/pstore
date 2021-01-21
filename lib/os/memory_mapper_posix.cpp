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
//===- lib/os/memory_mapper_posix.cpp -------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file memory_mapper_posix.cpp
/// \brief POSIX implementation of the platform-independent memory-mapped file

#include "pstore/os/memory_mapper.hpp"

#if !defined(_WIN32)

// Standard headers
#    include <cassert>
#    include <cerrno>
#    include <limits>
#    include <sstream>

// Platform headers
#    include <sys/mman.h>
#    include <unistd.h>

// pstore
#    include "pstore/config/config.hpp"
#    include "pstore/support/error.hpp"
#    include "pstore/support/quoted.hpp"

namespace {

    // 'offset' must be a multiple of the system page size and be safely castable to off_t.
    off_t checked_offset (std::uint64_t const offset) {
        using namespace pstore;

        if (offset % system_page_size ().get () != 0 ||
            offset >
                static_cast<std::make_unsigned<off_t>::type> (std::numeric_limits<off_t>::max ())) {
            raise (std::errc::invalid_argument, "mmap");
        }
        return static_cast<off_t> (offset);
    }

} // end anonymous namespace


namespace pstore {
    // MAP_ANONYMOUS isn't defined by POSIX, but both macOS and Linux support it.
    // Earlier versions of macOS provided the MAP_ANON name for the flag.
#    ifndef MAP_ANONYMOUS
#        define MAP_ANONYMOUS MAP_ANON
#    endif

    std::shared_ptr<std::uint8_t> aligned_valloc (std::size_t size, unsigned const align) {
        size += align - 1U;

        void * const ptr =
            ::mmap (nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == nullptr) {
            raise (pstore::errno_erc{errno}, "mmap");
        }

        auto const deleter = [ptr, size] (std::uint8_t * const) { ::munmap (ptr, size); };
        auto const mask = ~(std::uintptr_t{align} - 1);
        auto * const ptr_aligned = reinterpret_cast<std::uint8_t *> (
            (reinterpret_cast<std::uintptr_t> (ptr) + align - 1) & mask);
        return std::shared_ptr<std::uint8_t> (ptr_aligned, deleter);
    }

    //*                 _                                                _           *
    //*   ___ _   _ ___| |_ ___ _ __ ___    _ __   __ _  __ _  ___   ___(_)_______   *
    //*  / __| | | / __| __/ _ \ '_ ` _ \  | '_ \ / _` |/ _` |/ _ \ / __| |_  / _ \  *
    //*  \__ \ |_| \__ \ ||  __/ | | | | | | |_) | (_| | (_| |  __/ \__ \ |/ /  __/  *
    //*  |___/\__, |___/\__\___|_| |_| |_| | .__/ \__,_|\__, |\___| |___/_/___\___|  *
    //*       |___/                        |_|          |___/                        *
    // sysconf [static]
    // ~~~~~~~
    unsigned system_page_size::sysconf () {
#    ifdef PSTORE_HAVE_GETPAGESIZE
        int const result = getpagesize ();
        PSTORE_ASSERT (result > 0);
#    else
        long const result = ::sysconf (_SC_PAGESIZE);
        if (result == -1) {
            raise (errno_erc{errno}, "sysconf(_SC_PAGESIZE)");
        }
        PSTORE_ASSERT (result > 0 && result <= std::numeric_limits<unsigned>::max ());
#    endif // PSTORE_HAVE_GETPAGESIZE
        return static_cast<unsigned> (result);
    }



    // read_only
    // ~~~~~~~~~
    void memory_mapper_base::read_only_impl (void * const addr, std::size_t const len) {
        if (::mprotect (addr, len, PROT_READ) == -1) {
            raise (errno_erc{errno}, "mprotect");
        }
    }


    //*   _ __ ___   ___ _ __ ___   ___  _ __ _   _    _ __ ___   __ _ _ __  _ __   ___ _ __   *
    //*  | '_ ` _ \ / _ \ '_ ` _ \ / _ \| '__| | | |  | '_ ` _ \ / _` | '_ \| '_ \ / _ \ '__|  *
    //*  | | | | | |  __/ | | | | | (_) | |  | |_| |  | | | | | | (_| | |_) | |_) |  __/ |     *
    //*  |_| |_| |_|\___|_| |_| |_|\___/|_|   \__, |  |_| |_| |_|\__,_| .__/| .__/ \___|_|     *
    //*                                       |___/                   |_|   |_|                *
    // (ctor)
    // ~~~~~~
    memory_mapper::memory_mapper (file::file_handle & file, bool const write_enabled,
                                  std::uint64_t const offset, std::uint64_t const length)
            : memory_mapper_base (mmap (file, write_enabled, offset, length), write_enabled, offset,
                                  length) {}

    // (dtor)
    // ~~~~~~
    memory_mapper::~memory_mapper () noexcept = default;

    // mmap
    // ~~~~
    std::shared_ptr<void> memory_mapper::mmap (file::file_handle & file, bool const write_enabled,
                                               std::uint64_t const offset,
                                               std::uint64_t const length) {
        void * const ptr = ::mmap (nullptr, // base address
                                   length,
                                   PROT_READ | (write_enabled ? PROT_WRITE : 0), // protection flags
                                   MAP_SHARED, file.raw_handle (), checked_offset (offset));
        void const * const map_failed = MAP_FAILED; // NOLINT
        if (ptr == map_failed) {
            int const last_error = errno;
            std::ostringstream message;
            message << "Could not memory map file " << pstore::quoted (file.path ());
            raise (errno_erc{last_error}, message.str ());
        }

        return std::shared_ptr<void> (ptr, [length] (void * const p) {
            if (::munmap (p, length) == -1) {
                raise (errno_erc{errno}, "munmap");
            }
        });
    }

} // end namespace pstore
#endif //! defined (_WIN32)
