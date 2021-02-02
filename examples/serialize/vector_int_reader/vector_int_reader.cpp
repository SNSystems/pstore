//*                 _               _       _                        _            *
//* __   _____  ___| |_ ___  _ __  (_)_ __ | |_   _ __ ___  __ _  __| | ___ _ __  *
//* \ \ / / _ \/ __| __/ _ \| '__| | | '_ \| __| | '__/ _ \/ _` |/ _` |/ _ \ '__| *
//*  \ V /  __/ (__| || (_) | |    | | | | | |_  | | |  __/ (_| | (_| |  __/ |    *
//*   \_/ \___|\___|\__\___/|_|    |_|_| |_|\__| |_|  \___|\__,_|\__,_|\___|_|    *
//*                                                                               *
//===- examples/serialize/vector_int_reader/vector_int_reader.cpp ---------===//
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

    void read_one_int_at_a_time (container_type const & bytes) {
        auto reader = serialize::archive::make_reader (std::begin (bytes));
        int const v1 = serialize::read<int> (reader);
        int const v2 = serialize::read<int> (reader);
        std::cout << "Reading one int at a time produced " << v1 << ", " << v2 << '\n';
    }

    void read_an_array_of_ints (container_type const & bytes) {
        auto reader = serialize::archive::make_reader (std::begin (bytes));
        std::array<int, 2> arr;
        serialize::read (reader, gsl::make_span<int> (arr));
        std::cout << "Reading an array of ints produced " << arr[0] << ", " << arr[1] << '\n';
    }

    void read_a_series_of_ints (container_type const & bytes) {
        auto reader = serialize::archive::make_reader (std::begin (bytes));
        auto const v0 = serialize::read<int> (reader);
        auto const v1 = serialize::read<int> (reader);
        std::cout << "Reading a series of ints produced " << v0 << ", " << v1 << '\n';
    }
} // namespace

int main () {
    container_type const data{0x1e, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00};

    std::cout << "Reading two ints from the following input data:\n";
    dump (std::cout, std::begin (data), std::end (data));
    std::cout << '\n';

    read_one_int_at_a_time (data);
    read_an_array_of_ints (data);
    read_a_series_of_ints (data);
}
