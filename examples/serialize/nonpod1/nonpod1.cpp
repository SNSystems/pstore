//*                                    _ _  *
//*  _ __   ___  _ __  _ __   ___   __| / | *
//* | '_ \ / _ \| '_ \| '_ \ / _ \ / _` | | *
//* | | | | (_) | | | | |_) | (_) | (_| | | *
//* |_| |_|\___/|_| |_| .__/ \___/ \__,_|_| *
//*                   |_|                   *
//===- examples/serialize/nonpod1/nonpod1.cpp -----------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include <iostream>

#include "pstore/serialize/archive.hpp"
#include "pstore/serialize/types.hpp"

namespace {

    class foo {
    public:
        explicit constexpr foo (int const a) noexcept
                : a_{a} {}
        foo (foo const &) = default;

        // The presence of virtual methods in this class means that it is not "standard layout".
        virtual ~foo () noexcept = default;

        // Archival methods. The following two methods can be implemented on "non-standard layout"
        // types to enable reading from and writing to an archive. An alternative approach (which
        // also applies to standard layout types) is to write implement an explicit specialization
        // of pstore::serialize::serializer<Ty>.

        template <typename Archive>
        explicit foo (Archive & archive)
                : a_ (pstore::serialize::read<int> (archive)) {}

        template <typename Archive>
        auto write (Archive & archive) const -> typename Archive::result_type {
            return pstore::serialize::write (archive, a_);
        }

        foo & operator= (foo const &) = default;

        virtual int get () const { return a_; }

    private:
        int a_;
    };

    std::ostream & operator<< (std::ostream & os, foo const & f) {
        return os << "foo(" << f.get () << ')';
    }

} // end anonymous namespace

int main () {
    std::vector<std::uint8_t> bytes;

    // Serialize an instance of "foo" to the "bytes" vector.
    {
        pstore::serialize::archive::vector_writer writer{bytes};
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
