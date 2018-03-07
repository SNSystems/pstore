//*                        _ _      _    __                              _      *
//*  _ __   __ _ _ __ __ _| | | ___| |  / _| ___  _ __    ___  __ _  ___| |__   *
//* | '_ \ / _` | '__/ _` | | |/ _ \ | | |_ / _ \| '__|  / _ \/ _` |/ __| '_ \  *
//* | |_) | (_| | | | (_| | | |  __/ | |  _| (_) | |    |  __/ (_| | (__| | | | *
//* | .__/ \__,_|_|  \__,_|_|_|\___|_| |_|  \___/|_|     \___|\__,_|\___|_| |_| *
//* |_|                                                                         *
//===- include/pstore_cmd_util/parallel_for_each.hpp ----------------------===//
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

#ifndef PSTORE_CMD_UTIL_PARALLEL_FOR_EACH_HPP
#define PSTORE_CMD_UTIL_PARALLEL_FOR_EACH_HPP

#include <algorithm>
#include <future>
#include <iterator>
#include <vector>

#include "pstore_support/portab.hpp"

namespace pstore {
    namespace cmd_util {

        template <typename InputIt, typename UnaryFunction>
        void parallel_for_each (InputIt first, InputIt last, UnaryFunction fn) {
            auto it_distance = std::distance (first, last);
            if (it_distance <= 0) {
                return;
            }

            using difference_type = typename std::iterator_traits<InputIt>::difference_type;
            using udifference_type = typename std::make_unsigned<difference_type>::type;
            using wide_type =
                typename std::conditional<sizeof (udifference_type) >= sizeof (unsigned int),
                                          udifference_type, unsigned int>::type;
            auto num_elements = static_cast<wide_type> (it_distance);

            auto const num_threads = std::min (
                static_cast<wide_type> (std::max (std::thread::hardware_concurrency (), 1U)),
                num_elements);
            auto const partition_size = (num_elements + num_threads - 1) / num_threads;
            assert (partition_size * num_threads >= num_elements);

            std::vector<std::future<void>> futures;
            futures.reserve (num_threads);

            while (first != last) {
                wide_type const distance = std::min (partition_size, num_elements);
                auto next = first;
                assert (distance >= 0 &&
                        distance < static_cast<udifference_type> (
                                       std::numeric_limits<difference_type>::max ()));

                std::advance (next, static_cast<difference_type> (distance));
                using value_type = typename std::iterator_traits<InputIt>::value_type;

                // Create an asynchronous task which will sequentially process from 'first' to
                // 'next' invoking f() for each data member.
                auto per_thread_fn = [&fn](InputIt fst, InputIt lst) {
                    std::for_each (fst, lst, [&fn](value_type const & v) { fn (v); });
                };
                futures.push_back (std::async (std::launch::async, per_thread_fn, first, next));

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

    } // namespace cmd_util
} // namespace pstore

#endif // PSTORE_CMD_UTIL_PARALLEL_FOR_EACH_HPP
// eof: include/pstore_cmd_util/parallel_for_each.hpp
