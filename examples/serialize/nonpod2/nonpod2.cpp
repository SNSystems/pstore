//*                                    _ ____   *
//*  _ __   ___  _ __  _ __   ___   __| |___ \  *
//* | '_ \ / _ \| '_ \| '_ \ / _ \ / _` | __) | *
//* | | | | (_) | | | | |_) | (_) | (_| |/ __/  *
//* |_| |_|\___/|_| |_| .__/ \___/ \__,_|_____| *
//*                   |_|                       *
//===- examples/serialize/nonpod2/nonpod2.cpp -----------------------------===//
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
#include <iostream>

#include "pstore/serialize/archive.hpp"
#include "pstore/serialize/types.hpp"

namespace {
    class foo {
        friend struct pstore::serialize::serializer<foo>;

    public:
        constexpr explicit foo (int const a) noexcept
                : a_ (a) {}
        std::ostream & write (std::ostream & os) const;

    private:
        int a_;
    };

    std::ostream & foo::write (std::ostream & os) const { return os << "foo(" << a_ << ')'; }

    std::ostream & operator<< (std::ostream & os, foo const & f) { return f.write (os); }

} // end anonymous namespace

namespace pstore {
    namespace serialize {
        // A serializer for struct foo
        template <>
        struct serializer<foo> {
            using value_type = foo;

            // Writes an instance of foo to an archive. The data stream contains
            // a single int value.
            template <typename Archive>
            static auto write (Archive & archive, value_type const & value) ->
                typename Archive::result_type {

                return serialize::write (archive, value.a_);
            }

            // Reads an instance of foo from an archive.
            template <typename Archive>
            static void read (Archive & archive, value_type & sp) {
                new (&sp) foo (serialize::read<int> (archive));
            }
        };
    } // end namespace serialize
} // end namespace pstore

int main () {
    std::vector<std::uint8_t> bytes;
    {
        pstore::serialize::archive::vector_writer writer (bytes);
        {
            foo f (42);
            std::cout << "Writing: " << f << '\n';
            pstore::serialize::write (writer, f);
        }
        std::cout << "Wrote these bytes: " << writer << '\n';
    }
    {
        auto reader = pstore::serialize::archive::make_reader (std::begin (bytes));
        foo f = pstore::serialize::read<foo> (reader);
        std::cout << "Read: " << f << '\n';
    }
}
