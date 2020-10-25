//*  _                            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| *
//* | | | | | | | |_) | (_) | |  | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| *
//*             |_|                    *
//*   __                                      _    *
//*  / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                |___/                           *
//===- lib/exchange/import_fragment.cpp -----------------------------------===//
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
#include "pstore/exchange/import_fragment.hpp"

#include <type_traits>

namespace pstore {
    namespace exchange {
        namespace import {

            //*          _    _                            _      _     *
            //*  __ _ __| |__| |_ _ ___ ______  _ __  __ _| |_ __| |_   *
            //* / _` / _` / _` | '_/ -_|_-<_-< | '_ \/ _` |  _/ _| ' \  *
            //* \__,_\__,_\__,_|_| \___/__/__/ | .__/\__,_|\__\__|_||_| *
            //*                                |_|                      *
            // ctor
            // ~~~~
            address_patch::address_patch (gsl::not_null<database *> const db,
                                          extent<repo::fragment> const & ex) noexcept
                    : db_{db}
                    , fragment_extent_{ex} {}

            // operator()
            // ~~~~~~~~~~
            std::error_code address_patch::operator() (transaction_base * const transaction) {
                auto const compilations = index::get_index<trailer::indices::compilation> (*db_);

                auto fragment = repo::fragment::load (*transaction, fragment_extent_);
                for (repo::linked_definitions::value_type & l :
                     fragment->template at<repo::section_kind::linked_definitions> ()) {
                    auto const pos = compilations->find (*db_, l.compilation);
                    if (pos == compilations->end (*db_)) {
                        return error::no_such_compilation;
                    }
                    auto const compilation =
                        repo::compilation::load (transaction->db (), pos->second);
                    static_assert (std::is_unsigned<decltype (l.index)>::value,
                                   "index is not unsigned so we'll have to check for negative");
                    if (l.index >= compilation->size ()) {
                        return error::index_out_of_range;
                    }

                    // Compute the offset of the link.index definition from the start of the
                    // compilation's storage (c).
                    auto const offset =
                        reinterpret_cast<std::uintptr_t> (&(*compilation)[l.index]) -
                        reinterpret_cast<std::uintptr_t> (compilation.get ());

                    // Compute the address of the link.index definition.
                    l.pointer = typed_address<repo::compilation_member>::make (
                        pos->second.addr.to_address () + offset);
                }
                return {};
            }

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore
