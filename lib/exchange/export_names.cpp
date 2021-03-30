//===- lib/exchange/export_names.cpp --------------------------------------===//
//*                             _                                      *
//*   _____  ___ __   ___  _ __| |_   _ __   __ _ _ __ ___   ___  ___  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | '_ \ / _` | '_ ` _ \ / _ \/ __| *
//* |  __/>  <| |_) | (_) | |  | |_  | | | | (_| | | | | | |  __/\__ \ *
//*  \___/_/\_\ .__/ \___/|_|   \__| |_| |_|\__,_|_| |_| |_|\___||___/ *
//*           |_|                                                      *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/export_names.hpp"

#include <cassert>
#include <limits>

#include "pstore/exchange/export_ostream.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            //*                                             _            *
            //*  _ _  __ _ _ __  ___   _ __  __ _ _ __ _ __(_)_ _  __ _  *
            //* | ' \/ _` | '  \/ -_) | '  \/ _` | '_ \ '_ \ | ' \/ _` | *
            //* |_||_\__,_|_|_|_\___| |_|_|_\__,_| .__/ .__/_|_||_\__, | *
            //*                                  |_|  |_|         |___/  *
            // add
            // ~~~
            void name_mapping::add (address const addr) {
                PSTORE_ASSERT (names_.find (addr) == names_.end ());
                PSTORE_ASSERT (names_.size () <= std::numeric_limits<std::uint64_t>::max ());
                auto const index = static_cast<std::uint64_t> (names_.size ());
                names_[addr] = index;
            }

            // index
            // ~~~~~
            std::uint64_t name_mapping::index (typed_address<indirect_string> const addr) const {
                auto const pos = names_.find (addr.to_address ());
                PSTORE_ASSERT (pos != names_.end ());
                return pos->second;
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore
