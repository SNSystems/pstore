//*                 _                   _        _              *
//* __   _____  ___| |_ ___  _ __   ___| |_ _ __(_)_ __   __ _  *
//* \ \ / / _ \/ __| __/ _ \| '__| / __| __| '__| | '_ \ / _` | *
//*  \ V /  __/ (__| || (_) | |    \__ \ |_| |  | | | | | (_| | *
//*   \_/ \___|\___|\__\___/|_|    |___/\__|_|  |_|_| |_|\__, | *
//*                                                      |___/  *
//*                     _            *
//*  _ __ ___  __ _  __| | ___ _ __  *
//* | '__/ _ \/ _` |/ _` |/ _ \ '__| *
//* | | |  __/ (_| | (_| |  __/ |    *
//* |_|  \___|\__,_|\__,_|\___|_|    *
//*                                  *
//===- examples/serialize/vector_string_reader/vector_string_reader.cpp ---===//
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
#include <iostream>
#include <iomanip>

#include "pstore/serialize/archive.hpp"
#include "pstore/serialize/ios_state.hpp"
#include "pstore/serialize/standard_types.hpp"
#include "pstore/serialize/types.hpp"
#include "pstore/support/gsl.hpp"

using namespace pstore;

namespace {
    template <typename InputIterator>
    std::ostream & dump (std::ostream & os, InputIterator begin, InputIterator end) {
        pstore::serialize::ios_flags_saver flags{os};
        auto separator = "";
        os << std::setfill ('0') << std::hex;
        std::for_each (begin, end, [&](unsigned v) {
            os << separator << std::setw (2) << v;
            separator = " ";
        });
        return os;
    }

    using container_type = std::vector<std::uint8_t>;

    void read_one_string_at_a_time (container_type const & bytes) {
        auto reader = serialize::archive::make_reader (std::begin (bytes));
        auto v1 = serialize::read<std::string> (reader);
        auto v2 = serialize::read<std::string> (reader);
        std::cout << "Reading one string at a time produced \"" << v1 << "\" and \"" << v2
                  << "\"\n";
    }

    void read_an_array_of_strings (container_type const & bytes) {
        auto reader = serialize::archive::make_reader (std::begin (bytes));
        std::array<std::string, 2> arr;
        serialize::read (reader, gsl::make_span (arr));
        std::cout << "Reading an array of strings produced \"" << arr[0] << "\" and \"" << arr[1]
                  << "\"\n";
    }

    void read_a_series_of_strings (container_type const & bytes) {
        auto reader = serialize::archive::make_reader (std::begin (bytes));
        auto v0 = serialize::read<std::string> (reader);
        auto v1 = serialize::read<std::string> (reader);
        std::cout << "Reading a series of strings produced \"" << v0 << "\" and \"" << v1 << "\"\n";
    }
} // namespace

int main () {
    container_type data{'H', 'e', 'l', 'l', 'o', '\0', 'T', 'h', 'e', 'r', 'e', '\0'};

    std::cout << "Reading two strings from the following input data:\n";
    dump (std::cout, std::begin (data), std::end (data));
    std::cout << '\n';

    read_one_string_at_a_time (data);
    read_an_array_of_strings (data);
    read_a_series_of_strings (data);
}
