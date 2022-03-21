//===- unittests/os/test_memory_mapper.cpp --------------------------------===//
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
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/os/memory_mapper.hpp"

// Standard library includes
#include <algorithm>
#include <numeric>

// 3rd party includes
#include <gmock/gmock.h>

// pstore includes
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
