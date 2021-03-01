//===- examples/serialize/istream_reader/istream_reader.cpp ---------------===//
//*  _     _                                                 _            *
//* (_)___| |_ _ __ ___  __ _ _ __ ___    _ __ ___  __ _  __| | ___ _ __  *
//* | / __| __| '__/ _ \/ _` | '_ ` _ \  | '__/ _ \/ _` |/ _` |/ _ \ '__| *
//* | \__ \ |_| | |  __/ (_| | | | | | | | | |  __/ (_| | (_| |  __/ |    *
//* |_|___/\__|_|  \___|\__,_|_| |_| |_| |_|  \___|\__,_|\__,_|\___|_|    *
//*                                                                       *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <iostream>
#include <sstream>

// The serialization library
#include "pstore/serialize/archive.hpp"
#include "pstore/serialize/types.hpp"

namespace {
    class istream_reader {
    public:
        explicit istream_reader (std::istream & os)
                : os_ (os) {}

        // Reads a single instance of a standard-layout type T from the input stream and returns
        // the value extracted.
        template <typename T>
        void get (T & t) {
            static_assert (std::is_standard_layout<T>::value,
                           "istream_reader::get(T&) can only read standard-layout types");
            new (&t) T;
            os_ >> t;
        }

    private:
        std::istream & os_;
    };
} // namespace

int main () {
    std::istringstream iss{"3 73 127 179"};
    istream_reader reader{iss};

    for (auto count = 0U; count < 4U; ++count) {
        // Read a single integer from the istream_reader.
        auto value = pstore::serialize::read<int> (reader);
        std::cout << value << '\n';
    }
}
