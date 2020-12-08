//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===- tools/hamt_test/main.cpp -------------------------------------------===//
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
/// \brief A small utility which can be used to check the HAMT index.

#include <cmath>
#include <future>
#include <vector>

#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/tchar.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/hamt_map.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/transaction.hpp"
#include "pstore/support/parallel_for_each.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/utf.hpp" // for UTF-8 to UTF-16 conversion on Windows.

// Local includes
#include "./print.hpp"

using namespace pstore::command_line;

namespace {

    opt<std::string> data_file (positional, usage ("repository"),
                                desc ("Path of the pstore repository to use for index test."),
                                required);

} // end anonymous namespace

namespace {

    // A simple linear congruential random number generator from Numerical Recipes
    // Note that we're using a custom random number generator rather than the standard library
    // in order to guarantee that the numbers it produces are stable across different runs
    // and platforms.
    class random_number_generator {
    public:
        explicit random_number_generator (unsigned s = 0)
                : seed_{s % IM} {}

        double operator() () {
            seed_ = (IA * seed_ + IC) % IM;
            return seed_ / static_cast<double> (IM);
        }

    private:
        static unsigned const IM = 714025;
        static unsigned const IA = 1366;
        static unsigned const IC = 150889;

        unsigned seed_;
    };

} // end anonymous namespace

namespace {

    using random_list = std::vector<std::pair<pstore::index::digest, pstore::address>>;
    using less_map =
        std::map<pstore::index::digest, pstore::address, std::less<pstore::index::digest>>;
    using greater_map =
        std::map<pstore::index::digest, pstore::address, std::greater<pstore::index::digest>>;

    /// Generate the map from a random key to null address.
    /// \param num_keys the total number of random keys.
    random_list generate_random_keys_map (std::size_t const num_keys) {
        random_list l;
        random_number_generator random;

        auto uint32_rnum = [&random] () {
            return static_cast<std::uint64_t> (
                std::llrint (random () * std::numeric_limits<std::uint32_t>::max ()));
        };

        l.reserve (num_keys);
        for (auto ctr = std::size_t{0}; ctr < num_keys; ++ctr) {
            auto const r1 = uint32_rnum ();
            auto const r2 = uint32_rnum ();
            auto const r3 = uint32_rnum ();
            auto const r4 = uint32_rnum ();

            auto const key = pstore::index::digest{(r1 << 32) | r2, (r3 << 32) | r4};
            l.push_back (std::make_pair (key, pstore::address::null ()));
        }
        return l;
    }

    /// Generate the ordered map from a key to null address.
    /// \param num_keys the total number of keys minus two.
    /// \param step the difference between two consecutive keys.
    template <typename Map>
    Map generate_ordered_map (std::size_t num_keys, std::size_t const step) {
        Map map;
        map[std::numeric_limits<std::uint64_t>::max ()] = pstore::address::null ();
        do {
            std::uint64_t v = step * num_keys - num_keys;
            auto const key = pstore::index::digest{v, v};
            map[key] = pstore::address::null ();
        } while (num_keys--);
        return map;
    }

    /// Insert all keys of the maps into the database and update the mapped value to the expected
    /// database address.
    ///
    /// \param index A database index.
    /// \param maps It is an input and output parameter. The mapped values are updated once the
    ///        values are saved into the database. It stores the actual map in the database.
    template <typename Map>
    void insert (pstore::database & db, pstore::index::fragment_index & index, Map & maps) {
        // Start a transaction...
        auto transaction = pstore::begin (db);

        // Give the fixed size mapped value, since the tests are mainly focussed on
        // inserting/finding a key.
        std::array<std::uint8_t, 2> const value{{0, 1}};

        for (auto & kv : maps) {
            // Allocate space in the transaction for the value block
            auto addr = pstore::typed_address<std::uint8_t>::null ();
            std::shared_ptr<std::uint8_t> ptr;
            std::tie (ptr, addr) = transaction.alloc_rw<std::uint8_t> (value.size ());

            // Copy the value to the store.
            std::copy (std::begin (value), std::end (value), ptr.get ());

            // Update the mapped value.
            kv.second = addr.to_address ();

            // Add the key/value pair to the index.
            index.insert_or_assign (
                transaction, kv.first,
                make_extent (pstore::typed_address<pstore::repo::fragment> (addr.to_address ()),
                             value.size ()));
        }

        transaction.commit ();
    }

    /// Find all keys of the map (expected_results) in the database. Return true if all keys are in
    /// the database. Otherwise, return false.
    ///
    /// \param index  A database index.
    /// \param expected_results  A expected index which is saved in the database.
    /// \param test_name  A test name which is used to provide useful error information.
    /// \returns True if the test was successful, false otherwise.
    template <typename Map>
    bool find (pstore::database const & db, pstore::index::fragment_index const & index,
               Map const & expected_results, std::string const & test_name) {
        std::atomic<bool> is_found (true);
        auto check_key = [&db, &index, &test_name, &is_found] (typename Map::value_type value) {
            auto it = index.find (db, value.first);
            if (it == index.cend (db)) {
                print_cerr ("Test name:", test_name, " Error: ", value.first, ": not found");
                is_found = false;
            } else if (it->second.addr.to_address () != value.second) {
                print_cerr ("Test name:", test_name, " Error: The address of ", value.first,
                            " is not correct, the expected address: ", value.second);
                is_found = false;
            }
        };
        pstore::parallel_for_each (std::begin (expected_results), std::end (expected_results),
                                   check_key);
        return is_found.load ();
    }

} // end anonymous namespace

#ifdef _WIN32
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    PSTORE_TRY {
        parse_command_line_options (argc, argv, "Tests the pstore index code");

        pstore::database database (data_file.get (), pstore::database::access_mode::writable);
        database.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

        auto index = pstore::index::get_index<pstore::trailer::indices::fragment> (database);

        // In random number generator, the number is repeated after 300,000. The number of 2 ^ 18 is
        // closest to 300,000. Therefore, the num_keys is 2 ^ 18.
        auto const num_keys = std::size_t{1} << 18;
        auto const value_step = std::size_t{1} << 46; // (2 ^ 64 / 2 ^ 18)

        // Case 1a: generate the map with random keys.
        random_list map1 = generate_random_keys_map (num_keys);
        // Case 1b: insert the random keys.
        insert<random_list> (database, *index, map1);
        // Case 1c: find the random keys.
        if (!find<random_list> (database, *index, map1, "random key tests")) {
            exit_code = EXIT_FAILURE;
        }

        // Case 2a: generate the map with increasing keys.
        less_map map2 = generate_ordered_map<less_map> (num_keys, value_step);
        // Case 2b: insert the increasing key.
        insert<less_map> (database, *index, map2);
        // Case 2c: find the increasing key.
        if (!find<less_map> (database, *index, map2, "increasing key tests")) {
            exit_code = EXIT_FAILURE;
        }

        // Case 3a: generate the map with decreasing keys .
        greater_map map3 = generate_ordered_map<greater_map> (num_keys, value_step);
        // Case 3b: insert the decreasing key.
        insert<greater_map> (database, *index, map3);
        // Case 3c: find the decreasing key.
        if (!find<greater_map> (database, *index, map3, "decreasing key tests")) {
            exit_code = EXIT_FAILURE;
        }

        // TODO: test the following keys {0, num_keys*value_step, value_step-1,
        // num_keys*value_step-num_keys, 2*value_step-2...}

        database.close ();
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, {
        error_stream << NATIVE_TEXT ("An error occurred: ") << pstore::utf::to_native_string (ex.what ())
                     << std::endl;
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        error_stream << NATIVE_TEXT ("An unknown error occurred.") << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format on

    return exit_code;
}
