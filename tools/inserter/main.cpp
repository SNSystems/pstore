//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===- tools/inserter/main.cpp --------------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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

#ifdef _WIN32
#include <tchar.h>
#define NATIVE_TEXT(x) _TEXT (x)
#else
#include <unistd.h>

// On Windows, the TCHAR type may be either char or whar_t depending on the selected
// Unicode mode. Everywhere else, I need to add this type for compatibility.
using TCHAR = char;
// Similarly NATIVE_TEXT turns a string into a wide string on Windows.
#define NATIVE_TEXT(x) x
#endif

#include "pstore/cmd_util/cl/command_line.hpp"
#include "pstore/config/config.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/db_archive.hpp"
#include "pstore/core/hamt_map.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/transaction.hpp"
#include "pstore/serialize/standard_types.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/utf.hpp" // for UTF-8 to UTF-16 conversion on Windows.

#if PSTORE_HAS_SYS_KDEBUG_SIGNPOST_H
#include <sys/kdebug_signpost.h>
#endif

namespace {

#ifdef PSTORE_HAS_SYS_KDEBUG_SIGNPOST_H

    class profile_marker {
    public:
        profile_marker (std::uint32_t code, std::uintptr_t arg1 = 0, std::uintptr_t arg2 = 0,
                        std::uintptr_t arg3 = 0, std::uintptr_t arg4 = 0)
                : code_{code}
                , args_{{arg1, arg2, arg3, arg4}} {

            kdebug_signpost_start (code_, args_[0], args_[1], args_[2], args_[3]);
        }
        ~profile_marker () { kdebug_signpost_end (code_, args_[0], args_[1], args_[2], args_[3]); }

    private:
        std::uint32_t code_;
        std::array<std::uintptr_t, 4> args_;
    };

#else

    class profile_marker {
    public:
        profile_marker (std::uint32_t /*code*/, std::uintptr_t /*arg1*/ = 0,
                        std::uintptr_t /*arg2*/ = 0, std::uintptr_t /*arg3*/ = 0,
                        std::uintptr_t /*arg4*/ = 0) {}
        ~profile_marker () {}
    };

#endif // PSTORE_HAS_SYS_KDEBUG_SIGNPOST_H

#ifdef PSTORE_CPP_EXCEPTIONS
    auto & error_stream =
#if defined(_WIN32) && defined(_UNICODE)
        std::wcerr;
#else
        std::cerr;
#endif
#endif // PSTORE_CPP_EXCEPTIONS

} // namespace


namespace {

    // A simple linear congruential random number generator from Numerical Recipes
    class rng {
    public:
        explicit rng (unsigned s = 0)
                : seed_{s % IM} {}

        double operator() () {
            seed_ = (IA * seed_ + IC) % IM;
            return seed_ / double(IM);
        }

    private:
        static unsigned const IM = 714025;
        static unsigned const IA = 1366;
        static unsigned const IC = 150889;

        unsigned seed_;
    };

} // namespace

namespace {

    using digest_set = std::unordered_set<pstore::index::digest, pstore::index::u128_hash>;

    template <typename InputIt, typename UnaryFunction>
    void parallel_for_each (InputIt first, InputIt last, UnaryFunction fn) {
        auto num_elements = std::distance (first, last);
        auto const num_threads = std::max (std::thread::hardware_concurrency (), 1U);
        auto const partition_size = (num_elements + num_threads - 1) / num_threads;
        assert (partition_size * num_threads >= num_elements);

        std::vector<std::future<void>> futures;
        futures.reserve (num_threads);

        while (first != last) {
            auto const distance = std::min (partition_size, num_elements);
            auto next = first;
            std::advance (next, distance);

            // Create an asynchronous task which will sequentially process from 'first' to 'next'
            // invoking f() for each data member.
            futures.push_back (std::async (
                std::launch::async,
                [&fn](InputIt fst, InputIt lst) { std::for_each (fst, lst, fn); }, first, next));

            first = next;
            assert (num_elements >= distance);
            num_elements -= distance;
        }
        assert (num_elements == 0);
        assert (futures.size () == num_threads);

        // Join
        for (auto & f : futures) {
            f.wait ();
        }
    }

    void find (pstore::index::fragment_index const & index, digest_set const & keys) {
        profile_marker sgn (2);

        parallel_for_each (std::begin (keys), std::end (keys),
                           [&index](pstore::index::digest key) { index.find (key); });
    }

} // namespace


namespace {

    using namespace pstore::cmd_util;

    cl::opt<std::string>
        data_file (cl::Positional,
                   cl::desc ("Path of the pstore repository to use for index exercise."),
                   cl::Required);
} // namespace


#ifdef _WIN32
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    using pstore::utf::from_native_string;
    using pstore::utf::to_native_string;

    PSTORE_TRY {
        cl::ParseCommandLineOptions (argc, argv, "Exerices the pstore index code");

        pstore::database database (data_file.get (), pstore::database::access_mode::writable);

        auto index = pstore::index::get_index<pstore::trailer::indices::fragment> (database);

        // generate a large number of unique digests.
        // create a 1k value block (a simulated fragment)
        digest_set keys;
        std::vector<std::uint8_t> value{0, 1};

        {
            profile_marker sgn (1);

            auto const num_keys = std::size_t{300000};
            rng random;
            auto u64_random = [&random]() -> std::uint64_t {
                return static_cast<std::uint64_t> (
                    std::round (random () * std::numeric_limits<std::uint64_t>::max ()));
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

        find (*index, keys);

        {
            profile_marker sgn (3);
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
    PSTORE_CATCH (std::exception const & ex, {
        auto what = ex.what ();
        error_stream << NATIVE_TEXT ("An error occurred: ") << to_native_string (what)
                    << std::endl;
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        std::cerr << "An unknown error occurred." << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format on

    return exit_code;
}
