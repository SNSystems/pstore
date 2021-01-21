//*            _                                            _ _             *
//*   ___  ___| |_ _ __ ___  __ _ _ __ ___   __      ___ __(_) |_ ___ _ __  *
//*  / _ \/ __| __| '__/ _ \/ _` | '_ ` _ \  \ \ /\ / / '__| | __/ _ \ '__| *
//* | (_) \__ \ |_| | |  __/ (_| | | | | | |  \ V  V /| |  | | ||  __/ |    *
//*  \___/|___/\__|_|  \___|\__,_|_| |_| |_|   \_/\_/ |_|  |_|\__\___|_|    *
//*                                                                         *
//===- examples/serialize/ostream_writer/ostream_writer.cpp ---------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include <array>
#include <iostream>
#include <iterator>
#include <set>

// The serialization library
#include "pstore/serialize/archive.hpp"
#include "pstore/serialize/types.hpp"
#include "pstore/support/gsl.hpp"

namespace {

    class osw_policy {
    public:
        using result_type = pstore::serialize::archive::void_type;

        explicit osw_policy (std::ostream & os) noexcept
                : os_{os} {}

        // Writes an object of standard-layout type Ty to the output stream.
        // A return type of 'void_type' is used in the case that the archive writer policy does not
        // have a sensible value that it could return to the caller.
        template <typename Ty>
        auto put (Ty const & t) -> result_type {
            static_assert (std::is_standard_layout<Ty>::value, "Ty is not standard-layout");
            os_ << t << '\n';
            return {};
        }

        // Flushes the output stream.
        void flush () { os_ << std::flush; }

    private:
        std::ostream & os_;
    };

    class ostream_writer final : public pstore::serialize::archive::writer_base<osw_policy> {
    public:
        explicit ostream_writer (std::ostream & os)
                : writer_base<osw_policy> (osw_policy{os}) {}
    };

} // end anonymous namespace

int main () {
    // A simple function which will write a series of integer values to an instance ostream_writer.
    ostream_writer writer{std::cout};

    // The array of values that we'll be writing.
    std::array<int, 3> values{{179, 127, 73}};

    // Write the sequence of values as a span.
    pstore::serialize::write (writer, pstore::gsl::make_span (values));
}
