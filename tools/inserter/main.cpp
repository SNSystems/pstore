//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===- tools/inserter/main.cpp --------------------------------------------===//
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
/// \file main.cpp
/// \brief A small utility which can be used to profile the digest index.

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <future>
#include <iostream>
#include <memory>
#include <thread>
#include <unordered_set>

#include "pstore/cmd_util/command_line.hpp"
#include "pstore/cmd_util/tchar.hpp"
#include "pstore/config/config.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/db_archive.hpp"
#include "pstore/core/hamt_map.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/transaction.hpp"
#include "pstore/serialize/standard_types.hpp"
#include "pstore/support/parallel_for_each.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/utf.hpp" // for UTF-8 to UTF-16 conversion on Windows.

namespace {

    // A simple linear congruential random number generator from Numerical Recipes
    class rng {
    public:
        explicit rng (unsigned s = 0)
                : seed_{s % IM} {}

        double operator() () {
            seed_ = (IA * seed_ + IC) % IM;
            return seed_ / double (IM);
        }

    private:
        static unsigned const IM = 714025;
        static unsigned const IA = 1366;
        static unsigned const IC = 150889;

        unsigned seed_;
    };

    using digest_set = std::unordered_set<pstore::index::digest, pstore::index::u128_hash>;

    void find (pstore::database const & database, pstore::index::fragment_index const & index,
               digest_set const & keys) {
        pstore::parallel_for_each (
            std::begin (keys), std::end (keys),
            [&database, &index] (pstore::index::digest key) { index.find (database, key); });
    }

    using namespace pstore::cmd_util;

    cl::opt<std::string> data_file{
        cl::positional, cl::desc ("Path of the pstore repository to use for index exercise."),
        cl::required};

} // end anonymous namespace


#ifdef _WIN32
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    using pstore::utf::to_native_string;

    PSTORE_TRY {
        cl::parse_command_line_options (argc, argv, "Exercises the pstore index code");

        pstore::database database{data_file.get (), pstore::database::access_mode::writable};

        auto index = pstore::index::get_index<pstore::trailer::indices::fragment> (database);

        // generate a large number of unique digests.
        // create a 1k value block (a simulated fragment)
        digest_set keys;
        std::vector<std::uint8_t> value{0, 1};

        {
            auto const num_keys = std::size_t{300000};
            rng random;
            auto u64_random = [&random] () -> std::uint64_t {
                return (static_cast<std::uint64_t> (
                            std::round (random () * std::numeric_limits<std::uint32_t>::max ()))
                        << 32) |
                       static_cast<std::uint64_t> (
                           std::round (random () * std::numeric_limits<std::uint32_t>::max ()));
            };
            while (keys.size () < num_keys) {
                keys.insert (pstore::index::digest (u64_random (), u64_random ()));
            }

            auto const value_size = std::size_t{64};
            value.reserve (value_size);
            // A simple fibonnaci sequence generator.
            while (value.size () < value_size) {
                auto it = value.rbegin ();
                auto const a = *(it++), b = *(it++);
                value.push_back (a + b);
            }
        }

        find (database, *index, keys);

        {
            // Start a transaction...
            auto transaction = pstore::begin (database);

            for (auto & k : keys) {
                // Allocate space in the transaction for the value block
                auto addr = pstore::typed_address<std::uint8_t>::null ();
                std::shared_ptr<std::uint8_t> ptr;
                std::tie (ptr, addr) = transaction.alloc_rw<std::uint8_t> (value.size ());

                // Copy the value to the store.
                std::copy (std::begin (value), std::end (value), ptr.get ());

                // Add the key/value pair to the index.
                index->insert_or_assign (
                    transaction, k,
                    make_extent (pstore::typed_address<pstore::repo::fragment> (addr.to_address ()),
                                 value.size ()));
            }

            transaction.commit ();
        }

        database.close ();
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, { // clang-format on
        auto what = ex.what ();
        pstore::cmd_util::error_stream << NATIVE_TEXT ("An error occurred: ")
                                       << to_native_string (what)
                                       << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format off
    PSTORE_CATCH (..., { // clang-format on
        pstore::cmd_util::error_stream << NATIVE_TEXT ("An unknown error occurred.") << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format on

    return exit_code;
}
