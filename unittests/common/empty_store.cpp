//*                       _               _                  *
//*   ___ _ __ ___  _ __ | |_ _   _   ___| |_ ___  _ __ ___  *
//*  / _ \ '_ ` _ \| '_ \| __| | | | / __| __/ _ \| '__/ _ \ *
//* |  __/ | | | | | |_) | |_| |_| | \__ \ || (_) | | |  __/ *
//*  \___|_| |_| |_| .__/ \__|\__, | |___/\__\___/|_|  \___| *
//*                |_|        |___/                          *
//===- unittests/common/empty_store.cpp -----------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
#include "empty_store.hpp"
#include <cstdlib>
#include "pstore/support/error.hpp"

#ifdef _WIN32

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

#else

#include <sys/mman.h>

// MAP_ANONYMOUS isn't defined by POSIX, but both macOS and Linux support it.
// Earlier versions of macOS provided the MAP_ANON name for the flag.
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

std::shared_ptr<std::uint8_t> aligned_valloc (std::size_t size, unsigned align) {
    size += align - 1U;

    auto ptr = ::mmap (nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == nullptr) {
        raise (pstore::errno_erc{errno}, "mmap");
    }

    auto deleter = [ptr, size](std::uint8_t *) { ::munmap (ptr, size); };
    auto const mask = ~(std::uintptr_t{align} - 1);
    auto ptr_aligned = reinterpret_cast<std::uint8_t *> (
        (reinterpret_cast<std::uintptr_t> (ptr) + align - 1) & mask);
    return std::shared_ptr<std::uint8_t> (ptr_aligned, deleter);
}

#endif // _WIN32



constexpr std::size_t EmptyStore::file_size;
constexpr std::size_t EmptyStore::page_size_;

EmptyStore::EmptyStore ()
        : buffer_{aligned_valloc (file_size, page_size_)}
        , file_{std::make_shared<pstore::file::in_memory> (buffer_, file_size)} {
    pstore::database::build_new_store (*file_);
}

EmptyStore::~EmptyStore () {}
