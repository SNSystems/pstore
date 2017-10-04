//*                                    _ _  *
//*  _ __   ___  _ __  _ __   ___   __| / | *
//* | '_ \ / _ \| '_ \| '_ \ / _ \ / _` | | *
//* | | | | (_) | | | | |_) | (_) | (_| | | *
//* |_| |_|\___/|_| |_| .__/ \___/ \__,_|_| *
//*                   |_|                   *
//===- examples/serialize/nonpod1/nonpod1.cpp -----------------------------===//
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
#include <iostream>

#include "pstore/serialize/archive.hpp"
#include "pstore/serialize/types.hpp"

namespace {
    class foo {
    public:
        explicit foo (int a)
                : a_ (a) {}
        foo (foo const & ) = default;

        // The presence of virtual methods in this class means that it is not "standard layout".
        virtual ~foo () {}

        foo & operator= (foo const & ) = default;

        // Archival methods. The following two methods can be implemented on "non-standard layout" types to
        // enable reading from and writing to an archive. An alternative approach (which also
        // applies to standard layout types) is to write implement an explicit specialization
        // of pstore::serialize::serializer<Ty>.

        template <typename Archive>
        explicit foo (Archive & archive)
                : a_ (pstore::serialize::read<int> (archive)) {}

        template <typename Archive>
        auto write (Archive & archive) const -> typename Archive::result_type {
            return pstore::serialize::write (archive, a_);
        }

        virtual int get () const {
            return a_;
        }

    private:
        int a_;
    };

    std::ostream & operator<< (std::ostream & os, foo const & f) {
        return os << "foo(" << f.get () << ')';
    }
}

int main () {
    std::vector <std::uint8_t> bytes;

    // Serialize an instance of "foo" to the "bytes" vector.
    {
        pstore::serialize::archive::vector_writer writer {bytes};
        {
            foo f (42);
            std::cout << "Writing: " << f << '\n';
            pstore::serialize::write (writer, f);
        }
        std::cout << "Wrote these bytes: " << writer << '\n';
    }
    // Materialize an instance of "foo" from the "bytes" container.
    {
        auto reader = pstore::serialize::archive::make_reader (std::begin (bytes));
        auto f = pstore::serialize::read<foo> (reader);
        std::cout << "Read: " << f << '\n';
    }
}
// eof: examples/serialize/nonpod1/nonpod1.cpp
