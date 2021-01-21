//*                        _ _      _    __                              _      *
//*  _ __   __ _ _ __ __ _| | | ___| |  / _| ___  _ __    ___  __ _  ___| |__   *
//* | '_ \ / _` | '__/ _` | | |/ _ \ | | |_ / _ \| '__|  / _ \/ _` |/ __| '_ \  *
//* | |_) | (_| | | | (_| | | |  __/ | |  _| (_) | |    |  __/ (_| | (__| | | | *
//* | .__/ \__,_|_|  \__,_|_|_|\___|_| |_|  \___/|_|     \___|\__,_|\___|_| |_| *
//* |_|                                                                         *
//===- include/pstore/support/parallel_for_each.hpp -----------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#ifndef PSTORE_SUPPORT_PARALLEL_FOR_EACH_HPP
#define PSTORE_SUPPORT_PARALLEL_FOR_EACH_HPP

#include <algorithm>
#include <future>
#include <iterator>
#include <vector>

#include "pstore/support/assert.hpp"

namespace pstore {

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

        auto const num_threads =
            std::min (static_cast<wide_type> (std::max (std::thread::hardware_concurrency (), 1U)),
                      num_elements);

        // The number of work items to be processed by each worker thread.
        auto const partition_size = (num_elements + num_threads - 1) / num_threads;
        PSTORE_ASSERT (partition_size * num_threads >= num_elements);

        // TODO: use small_vector<>.
        std::vector<std::future<void>> futures;
        futures.reserve (num_threads);

        while (first != last) {
            wide_type const distance = std::min (partition_size, num_elements);
            auto next = first;
            PSTORE_ASSERT (distance >= 0 &&
                           distance < static_cast<udifference_type> (
                                          std::numeric_limits<difference_type>::max ()));

            std::advance (next, static_cast<difference_type> (distance));
            using value_type = typename std::iterator_traits<InputIt>::value_type;

            // Create an asynchronous task which will sequentially process from 'first' to
            // 'next' invoking f() for each data member.
            auto per_thread_fn = [&fn] (InputIt fst, InputIt lst) {
                std::for_each (fst, lst, [&fn] (value_type const & v) { fn (v); });
            };
            futures.emplace_back (std::async (std::launch::async, per_thread_fn, first, next));

            first = next;
            PSTORE_ASSERT (num_elements >= distance);
            num_elements -= distance;
        }
        PSTORE_ASSERT (num_elements == 0 && futures.size () <= num_threads);

        // Join
        for (auto & f : futures) {
            PSTORE_ASSERT (f.valid ());
            // Note that future::get<> is normally used to retreive the future's result, but
            // here I'm calling it to ensure that an exception stored in the future's state is
            // raised.
            f.get ();
        }
    }

} // namespace pstore

#endif // PSTORE_SUPPORT_PARALLEL_FOR_EACH_HPP
