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
//===- unittests/os/test_memory_mapper.cpp --------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
#include "pstore/os/memory_mapper.hpp"

// Standard library
#include <algorithm>
#include <numeric>

// 3rd party
#include "gmock/gmock.h"

// pstore
#include "pstore/os/file.hpp"
#include "pstore/support/error.hpp"

TEST (MemoryMapper, MemoryMapThenCheckFileContents) {
    using ::testing::ContainerEq;

    pstore::file::file_handle file;
    file.open (pstore::file::file_handle::temporary ());

    std::size_t const size = pstore::system_page_size ().get ();
    ASSERT_GT (size, 0U);

    file.seek (size - 1U);
    file.write (0);
    {
        pstore::memory_mapper mm{file,  // backing file
                                 true,  // writable?
                                 0U,    // offset
                                 size}; // number of bytes to map

        EXPECT_EQ (size, mm.size ());
        EXPECT_EQ (0U, mm.offset ());

        // Flood the memory mapped file with values.
        auto ptr = std::static_pointer_cast<std::uint8_t> (mm.data ());
        std::iota (ptr.get (), ptr.get () + size, std::uint8_t{0});
    }

    // Now read back the contents of the file.
    file.seek (0);
    std::vector<std::uint8_t> contents (size);
    file.read_span (pstore::gsl::make_span (contents));

    // Now check that the file contains thevalues we wrote to it.
    std::vector<std::uint8_t> expected (size);
    std::iota (expected.begin (), expected.end (), std::uint8_t{0});
    EXPECT_THAT (expected, ContainerEq (contents));
}
