//*                                    _                   _ _   _       _       *
//*  _ __   ___  _ __  _ __   ___   __| |  _ __ ___  _   _| | |_(_)_ __ | | ___  *
//* | '_ \ / _ \| '_ \| '_ \ / _ \ / _` | | '_ ` _ \| | | | | __| | '_ \| |/ _ \ *
//* | | | | (_) | | | | |_) | (_) | (_| | | | | | | | |_| | | |_| | |_) | |  __/ *
//* |_| |_|\___/|_| |_| .__/ \___/ \__,_| |_| |_| |_|\__,_|_|\__|_| .__/|_|\___| *
//*                   |_|                                         |_|            *
//===- examples/serialize/nonpod_multiple/nonpod_multiple.cpp -------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <iostream>
#include <new>

#include "pstore/serialize/archive.hpp"
#include "pstore/serialize/types.hpp"
#include "pstore/support/gsl.hpp"

namespace {

    class foo {
        friend struct pstore::serialize::serializer<foo>;

    public:
        foo () = default;
        constexpr explicit foo (int const a) noexcept
                : a_ (a) {}
        std::ostream & write (std::ostream & os) const;

    private:
        int a_ = 0;
    };

    std::ostream & foo::write (std::ostream & os) const { return os << "foo(" << a_ << ')'; }

    std::ostream & operator<< (std::ostream & os, foo const & f) { return f.write (os); }

} // end anonymous namespace

namespace pstore {
    namespace serialize {

        // A serializer for struct foo
        template <>
        struct serializer<foo> {
            // Writes an instance of foo to an archive. The data stream contains
            // a single int value.
            template <typename Archive>
            static auto write (Archive & archive, foo const & value) ->
                typename Archive::result_type {
                return serialize::write (archive, value.a_);
            }

            // Reads an instance of foo from an archive. To do this, we read an integer from
            // the supplied archive and use it to construct a new foo instance into the
            // uninitialized memory supplied by the caller.
            template <typename Archive>
            static void read (Archive & archive, foo & sp) {
                new (&sp) foo (serialize::read<int> (archive));
            }
        };

    } // end namespace serialize
} // end namespace pstore

int main () {
    // This is the container into which the vector_writer will place the serialized data.
    std::vector<std::uint8_t> bytes;

    // First write an array of "foo" instance to the "bytes" container.
    {
        pstore::serialize::archive::vector_writer writer (bytes);
        std::array<foo, 2> src{{foo{37}, foo{42}}};

        std::cout << "Writing: ";
        std::copy (std::begin (src), std::end (src), std::ostream_iterator<foo> (std::cout, " "));
        std::cout << std::endl;

        pstore::serialize::write (writer, pstore::gsl::make_span (src));
        std::cout << "Wrote these bytes: " << writer << '\n';
    }

    // Now use the contents of the "bytes" vector to materialize two foo instances.
    {
        auto reader = pstore::serialize::archive::make_reader (std::begin (bytes));

        std::array<foo, 2> dest;
        pstore::serialize::read (reader, pstore::gsl::make_span (dest));

        std::cout << "Read: ";
        std::copy (std::begin (dest), std::end (dest), std::ostream_iterator<foo> (std::cout, " "));
        std::cout << std::endl;
    }
}
