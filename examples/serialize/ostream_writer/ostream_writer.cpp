//*            _                                            _ _             *
//*   ___  ___| |_ _ __ ___  __ _ _ __ ___   __      ___ __(_) |_ ___ _ __  *
//*  / _ \/ __| __| '__/ _ \/ _` | '_ ` _ \  \ \ /\ / / '__| | __/ _ \ '__| *
//* | (_) \__ \ |_| | |  __/ (_| | | | | | |  \ V  V /| |  | | ||  __/ |    *
//*  \___/|___/\__|_|  \___|\__,_|_| |_| |_|   \_/\_/ |_|  |_|\__\___|_|    *
//*                                                                         *
//===- examples/serialize/ostream_writer/ostream_writer.cpp ---------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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

        osw_policy (std::ostream & os)
                : os_ (os) {}

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
                : writer_base<osw_policy> (os) {}
    };
} // namespace

int main () {
    // A simple function which will write a series of integer values to an instance ostream_writer.
    ostream_writer writer{std::cout};

    // The array of values that we'll be writing.
    std::array<int, 3> values{{179, 127, 73}};

    // Write the sequence of values as a span.
    pstore::serialize::write (writer, pstore::gsl::make_span (values));
}
