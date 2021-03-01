//===- examples/serialize/vector_string_reader/vector_string_reader.cpp ---===//
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
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <iostream>
#include <iomanip>

#include "pstore/serialize/archive.hpp"
#include "pstore/serialize/standard_types.hpp"
#include "pstore/serialize/types.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/ios_state.hpp"

using namespace pstore;

namespace {
    template <typename InputIterator>
    std::ostream & dump (std::ostream & os, InputIterator begin, InputIterator end) {
        pstore::ios_flags_saver const _{os};
        auto separator = "";
        os << std::setfill ('0') << std::hex;
        std::for_each (begin, end, [&] (unsigned const v) {
            os << separator << std::setw (2) << v;
            separator = " ";
        });
        return os;
    }

    using container_type = std::vector<std::uint8_t>;

    void read_one_string_at_a_time (container_type const & bytes) {
        auto reader = serialize::archive::make_reader (std::begin (bytes));
        auto const v1 = serialize::read<std::string> (reader);
        auto const v2 = serialize::read<std::string> (reader);
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
        auto const v0 = serialize::read<std::string> (reader);
        auto const v1 = serialize::read<std::string> (reader);
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
