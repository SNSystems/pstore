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
//===- unittests/pstore/test_memory_mapper.cpp ----------------------------===//
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
#include "pstore/core/memory_mapper.hpp"

#include <cstring>
#include <memory>

// 3rd party
#include "gtest/gtest.h"
#include "gmock/gmock.h"

/// pstore
#include "pstore/support/error.hpp"
#include "pstore/support/file.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/make_unique.hpp"

#ifndef _WIN32
#include <errno.h>
#include <unistd.h>
#endif


namespace {
    class MemoryMapperFixture : public ::testing::Test {
    public:
        MemoryMapperFixture ()
                : file_{std::make_unique<pstore::file::file_handle> ()} {

            file_->open (pstore::file::file_handle::temporary ());
        }

    protected:
        static std::size_t page_size ();

        pstore::file::file_handle & file () { return *file_; }

    private:
        std::unique_ptr<pstore::file::file_handle> file_;
    };

    std::size_t MemoryMapperFixture::page_size () {
#ifdef _WIN32
        SYSTEM_INFO system_info;
        ::GetSystemInfo (&system_info);
        return static_cast<std::size_t> (system_info.dwPageSize);
#else
        long result = ::sysconf (_SC_PAGESIZE);
        if (result == -1) {
            raise (pstore::errno_erc{errno}, "sysconf(_SC_PAGESIZE)");
        }
        assert (result >= 0);
        return static_cast<unsigned long> (result);
#endif
    }
} // namespace

TEST_F (MemoryMapperFixture, MemoryMapThenCheckFileContents) {

    /// Linux: the 'offset' parameter must be a multiple of the value returned by
    /// sysconf(_SC_PAGESIZE)
    ///
    /// Windows: the 'offset' parameter must be a multiple of the allocation granularity given by
    /// SYSTEM_INFO structure filled in by a call to GetSystemInfo().

    std::size_t const size = this->page_size ();
    pstore::file::file_handle & backing_store = this->file ();
    {
        backing_store.seek (size);
        backing_store.write (0);

        pstore::memory_mapper mm (backing_store, // backing file
                                  true,          // writable?
                                  0,             // offset
                                  size);         // number of bytes to map

        EXPECT_EQ (size, mm.size ());
        EXPECT_EQ (0U, mm.offset ());

        auto ptr = std::static_pointer_cast<std::uint8_t> (mm.data ());
        auto ptr8 = ptr.get ();
        std::memset (ptr8, 0, size);
        ptr8[0] = 0xFF;
        ptr8[size - 1] = 0xFF;
    }

    backing_store.seek (0);
    std::vector<std::uint8_t> contents (size);
    backing_store.read_span (::pstore::gsl::make_span (contents));

    std::vector<std::uint8_t> expected (size);
    std::fill (expected.begin (), expected.end (), std::uint8_t{0});
    expected[0] = 0xFF;
    expected[size - 1] = 0xFF;

    EXPECT_THAT (expected, ::testing::ContainerEq (contents));
}

// eof: unittests/pstore/test_memory_mapper.cpp
