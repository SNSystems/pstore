//===- lib/exchange/export_strings.cpp ------------------------------------===//
//*                             _         _        _                  *
//*   _____  ___ __   ___  _ __| |_   ___| |_ _ __(_)_ __   __ _ ___  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| / __| __| '__| | '_ \ / _` / __| *
//* |  __/>  <| |_) | (_) | |  | |_  \__ \ |_| |  | | | | | (_| \__ \ *
//*  \___/_/\_\ .__/ \___/|_|   \__| |___/\__|_|  |_|_| |_|\__, |___/ *
//*           |_|                                          |___/      *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/export_strings.hpp"

#include <limits>

namespace pstore {
    namespace exchange {
        namespace export_ns {

            //*     _       _                                  _            *
            //*  __| |_ _ _(_)_ _  __ _   _ __  __ _ _ __ _ __(_)_ _  __ _  *
            //* (_-<  _| '_| | ' \/ _` | | '  \/ _` | '_ \ '_ \ | ' \/ _` | *
            //* /__/\__|_| |_|_||_\__, | |_|_|_\__,_| .__/ .__/_|_||_\__, | *
            //*                   |___/             |_|  |_|         |___/  *
            // add
            // ~~~
            std::uint64_t string_mapping::add (address const addr) {
                PSTORE_ASSERT (strings_.find (addr) == strings_.end ());
                PSTORE_ASSERT (strings_.size () <= std::numeric_limits<std::uint64_t>::max ());
                auto const index = static_cast<std::uint64_t> (strings_.size ());
                strings_[addr] = index;
                return index;
            }

            // index
            // ~~~~~
            std::uint64_t string_mapping::index (typed_address<indirect_string> const addr) const {
                auto const pos = strings_.find (addr.to_address ());
                PSTORE_ASSERT (pos != strings_.end ());
                return pos->second;
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore
