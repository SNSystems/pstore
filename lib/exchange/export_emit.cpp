//*                             _                    _ _    *
//*   _____  ___ __   ___  _ __| |_    ___ _ __ ___ (_) |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __|  / _ \ '_ ` _ \| | __| *
//* |  __/>  <| |_) | (_) | |  | |_  |  __/ | | | | | | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__|  \___|_| |_| |_|_|\__| *
//*           |_|                                           *
//===- lib/exchange/export_emit.cpp ---------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/export_emit.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            void emit_digest (ostream_base & os, uint128 const d) {
                std::array<char, uint128::hex_string_length> hex;
                auto const out = d.to_hex (hex.begin ());
                (void) out;
                PSTORE_ASSERT (out == hex.end ());
                os << '"';
                details::write_span (os, gsl::make_span (hex));
                os << '"';
            }


            void emit_string (ostream_base & os, raw_sstring_view const & view) {
                emit_string (os, std::begin (view), std::end (view));
            }

            ostream_base & show_string (ostream_base & os, pstore::database const & db,
                                        pstore::typed_address<pstore::indirect_string> const addr,
                                        bool const comments) {
                if (comments) {
                    auto const str = serialize::read<pstore::indirect_string> (
                        serialize::archive::database_reader{db, addr.to_address ()});
                    os << R"( //")" << str << '"';
                }
                return os;
            }


        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore
