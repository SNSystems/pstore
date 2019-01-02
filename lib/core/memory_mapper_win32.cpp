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
//===- lib/core/memory_mapper_win32.cpp -----------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
/// \file memory_mapper_win32.cpp
/// \brief Win32 implementation of the platform-independent memory-mapped file
#include "pstore/core/memory_mapper.hpp"

#if defined(_WIN32)
#include <cassert>
#include <iomanip>
#include <limits>
#include <sstream>
#include <system_error>

#include "pstore/support/quoted_string.hpp"

namespace {
    constexpr unsigned dword_bits = sizeof (DWORD) * 8;

    inline constexpr auto high4 (std::uint64_t v) -> DWORD {
        return static_cast<DWORD> (v >> dword_bits);
    }
    inline constexpr auto low4 (std::uint64_t v) -> DWORD {
        return v & ((std::uint64_t{1} << dword_bits) - 1U);
    }

    /// A simple RAII-style class for managing a Win32 file mapping.
    class file_mapping {
    public:
        file_mapping (pstore::file::file_handle & file, bool write_enabled,
                      std::size_t mapping_size);
        ~file_mapping () noexcept;

        HANDLE handle () { return mapping_; }

        // No copying, no assignment.
        file_mapping (file_mapping const &) = delete;
        file_mapping & operator= (file_mapping const &) = delete;

    private:
        HANDLE mapping_;
    };

    file_mapping::file_mapping (pstore::file::file_handle & file, bool write_enabled,
                                std::size_t mapping_size)
            : mapping_ (::CreateFileMapping (file.raw_handle (),
                                             nullptr, // security attributes
                                             write_enabled ? PAGE_READWRITE : PAGE_READONLY,
                                             high4 (mapping_size), low4 (mapping_size),
                                             nullptr)) { // name
        if (mapping_ == nullptr) {
            DWORD const last_error = ::GetLastError ();
            std::ostringstream message;
            message << "CreateFileMapping failed for " << std::quoted (file.path ());
            raise (::pstore::win32_erc (last_error), message.str ());
        }
    }

    file_mapping::~file_mapping () noexcept {
        assert (mapping_ != nullptr);
        ::CloseHandle (mapping_);
    }
} // namespace


namespace pstore {

    std::shared_ptr<std::uint8_t> aligned_valloc (std::size_t size, unsigned align) {
        size += align - 1U;

        auto ptr = reinterpret_cast<std::uint8_t *> (
            ::VirtualAlloc (nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (ptr == nullptr) {
            DWORD const last_error = ::GetLastError ();
            raise (pstore::win32_erc (last_error), "VirtualAlloc");
        }

        auto deleter = [ptr](std::uint8_t *) { ::VirtualFree (ptr, 0, MEM_RELEASE); };
        auto const mask = ~(std::uintptr_t{align} - 1);
        auto ptr_aligned = reinterpret_cast<std::uint8_t *> (
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
        SYSTEM_INFO system_info;
        ::GetSystemInfo (&system_info);
        auto const result = system_info.dwPageSize;
        assert (result > 0 && result <= std::numeric_limits<unsigned>::max ());
        return static_cast<unsigned> (result);
    }


    //*   _ __ ___   ___ _ __ ___   ___  _ __ _   _    _ __ ___   __ _ _ __  _ __   ___ _ __   *
    //*  | '_ ` _ \ / _ \ '_ ` _ \ / _ \| '__| | | |  | '_ ` _ \ / _` | '_ \| '_ \ / _ \ '__|  *
    //*  | | | | | |  __/ | | | | | (_) | |  | |_| |  | | | | | | (_| | |_) | |_) |  __/ |     *
    //*  |_| |_| |_|\___|_| |_| |_|\___/|_|   \__, |  |_| |_| |_|\__,_| .__/| .__/ \___|_|     *
    //*                                       |___/                   |_|   |_|                *
    // read_only_impl
    // ~~~~~~~~~~~~~~
    void memory_mapper_base::read_only_impl (void * addr, std::size_t len) {
        DWORD old_protect = 0;
        if (::VirtualProtect (addr, len, PAGE_READONLY, &old_protect) == 0) {
            DWORD const last_error = ::GetLastError ();
            raise (::pstore::win32_erc (last_error), "VirtualProtect");
        }
    }

    // (ctor)
    // ~~~~~~
    memory_mapper::memory_mapper (file::file_handle & file, bool write_enabled,
                                  std::uint64_t offset, std::uint64_t length)
            : memory_mapper_base (mmap (file, write_enabled, offset, length), write_enabled, offset,
                                  length) {}

    // (dtor)
    // ~~~~~~
    memory_mapper::~memory_mapper () noexcept = default;

    // mmap [static]
    // ~~~~~~~~~~~~~
    std::shared_ptr<void> memory_mapper::mmap (file::file_handle & file, bool write_enabled,
                                               std::uint64_t offset, std::uint64_t length) {
        file_mapping mapping (file, write_enabled, offset + length);
        void * mapped_ptr =
            ::MapViewOfFile (mapping.handle (), write_enabled ? FILE_MAP_WRITE : FILE_MAP_READ,
                             high4 (offset), low4 (offset), length);
        if (mapped_ptr == nullptr) {
            DWORD const last_error = ::GetLastError ();

            std::ostringstream message;
            message << "Could not map view of file " << std::quoted (file.path ());
            raise (::pstore::win32_erc (last_error), message.str ());
        }

        auto unmap_deleter = [](void * p) {
            if (::UnmapViewOfFile (p) == 0) {
                DWORD const last_error = ::GetLastError ();
                raise (::pstore::win32_erc (last_error), "UnmapViewOfFile");
            }
        };

        return std::shared_ptr<void> (mapped_ptr, unmap_deleter);
    }

} // namespace pstore

#endif // defined (_WIN32)
