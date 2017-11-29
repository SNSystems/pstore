//*                 _     _            *
//*   __ _ _ __ ___| |__ (_)_   _____  *
//*  / _` | '__/ __| '_ \| \ \ / / _ \ *
//* | (_| | | | (__| | | | |\ V /  __/ *
//*  \__,_|_|  \___|_| |_|_| \_/ \___| *
//*                                    *
//===- unittests/pstore/serialize/test_archive.cpp ------------------------===//
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
#include "pstore/serialize/archive.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <type_traits>

#include "gmock/gmock.h"
#include "pstore_support/gsl.hpp"
#include "check_for_error.hpp"

TEST (SerializeArchiveVectorWriter, Write1Byte) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve (1);

    pstore::serialize::archive::vector_writer writer (bytes);
    std::uint8_t v{251};
    writer.put (v);

    auto begin = std::begin (writer);
    auto end = std::end (writer);
    ASSERT_EQ (1, std::distance (begin, end));
    EXPECT_EQ (1U, writer.bytes_consumed ());
    EXPECT_EQ (1U, writer.bytes_produced ());
    EXPECT_EQ (v, *begin);
}

TEST (SerializeArchiveVectorWriter, WriteAnInt) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve (sizeof (int));
    pstore::serialize::archive::vector_writer writer (bytes);
    writer.put (42);

    // Check that we wrote sizeof (int) bytes.
    auto begin = std::begin (writer);
    auto end = std::end (writer);

    using difference_type = std::iterator_traits<decltype (begin)>::difference_type;
    ASSERT_EQ (difference_type{sizeof (int)}, std::distance (begin, end));
    EXPECT_EQ (bytes.size (), writer.bytes_consumed ());
    EXPECT_EQ (bytes.size (), writer.bytes_produced ());

    // Now coax the value back out of the byte array that the vector_writer has accumulated.
    std::uint8_t content[sizeof (int)];
    std::copy (begin, end, content);
    int value;
    std::memcpy (&value, content, sizeof (value));
    EXPECT_EQ (42, value);
}



TEST (SerializeArchiveNull, WriteAnInt) {
    pstore::serialize::archive::null writer;
    writer.put (42);
    EXPECT_EQ (sizeof (int), writer.bytes_consumed ());
    EXPECT_EQ (sizeof (int), writer.bytes_produced ());
}

TEST (SerializeArchiveNull, WriteTwoInts) {
    pstore::serialize::archive::null writer;
    EXPECT_EQ (0U, writer.bytes_consumed ());
    writer.put (42);
    EXPECT_EQ (sizeof (int), writer.bytes_consumed ());
    writer.put (43);
    EXPECT_EQ (sizeof (int) * 2, writer.bytes_consumed ());
    EXPECT_EQ (sizeof (int) * 2, writer.bytes_produced ());
}

TEST (SerializeArchiveNull, WriteSpan) {
    pstore::serialize::archive::null writer;
    std::array<int, 2> arr{{13, 17}};
    writer.putn (::pstore::gsl::make_span (arr));
    EXPECT_EQ (sizeof (int) * 2, writer.bytes_consumed ());
    EXPECT_EQ (sizeof (int) * 2, writer.bytes_produced ());
}


TEST (SerializeBufferReader, ReadByte) {
    std::array<std::uint8_t, 1> buffer{{28}};
    pstore::serialize::archive::buffer_reader reader (::pstore::gsl::make_span (buffer));
    EXPECT_EQ (28, reader.get<std::uint8_t> ());
}

TEST (SerializeBufferReader, ReadPastEnd) {
    std::array<std::uint8_t, 1> buffer{{28}};
    pstore::serialize::archive::buffer_reader reader (::pstore::gsl::make_span (buffer));
    check_for_error ([&reader]() { reader.get<std::uint16_t> (); }, std::errc::no_buffer_space);
}

// eof: unittests/pstore/serialize/test_archive.cpp
