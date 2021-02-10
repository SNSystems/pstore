//*      _ _                  _             *
//*   __| | |__   __   ____ _| |_   _  ___  *
//*  / _` | '_ \  \ \ / / _` | | | | |/ _ \ *
//* | (_| | |_) |  \ V / (_| | | |_| |  __/ *
//*  \__,_|_.__/    \_/ \__,_|_|\__,_|\___| *
//*                                         *
//===- lib/dump/db_value.cpp ----------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/dump/db_value.hpp"

#include <chrono>
#include <ctime>

#include "pstore/core/generation_iterator.hpp"

namespace pstore {
    namespace dump {

        bool address::default_expanded_ = false;

        // write_impl
        // ~~~~~~~~~~
        std::ostream & address::write_impl (std::ostream & os, indent const & indent) const {
            return this->real_value ()->write_impl (os, indent);
        }
        std::wostream & address::write_impl (std::wostream & os, indent const & indent) const {
            return this->real_value ()->write_impl (os, indent);
        }

        // real_value
        // ~~~~~~~~~~
        value_ptr address::real_value () const {
            if (!value_) {
                if (expanded_) {
                    auto const full = std::make_shared<object> (object::container{
                        {"segment", make_value (addr_.segment ())},
                        {"offset", make_value (addr_.offset ())},
                    });
                    full->compact ();
                    value_ = full;
                } else {
                    value_ = make_value (addr_.absolute ());
                }
            }
            PSTORE_ASSERT (value_.get () != nullptr);
            return value_;
        }


        // *************
        //* make_value *
        // *************
        value_ptr make_value (header const & header) {
            auto const & version = header.version ();
            return make_value (object::container{
                {"signature1",
                 make_value (std::begin (header.a.signature1), std::end (header.a.signature1))},
                {"signature2", make_value (header.a.signature2)},
                {"version", make_value (std::begin (version), std::end (version))},
                {"id", make_value (header.id ())},
                {"crc", make_value (header.crc)},
                {"footer_pos", make_value (header.footer_pos.load ())},
            });
        }

        value_ptr make_value (trailer const & trailer, bool const no_times) {
            return make_value (object::container{
                {"signature1",
                 make_value (std::begin (trailer.a.signature1), std::end (trailer.a.signature1))},
                {"generation", make_value (trailer.a.generation.load ())},
                {"size", make_value (trailer.a.size.load ())},
                {"time", make_time (trailer.a.time, no_times)},
                {"prev_generation", make_value (trailer.a.prev_generation)},
                {"indices", make_value (std::begin (trailer.a.index_records),
                                        std::end (trailer.a.index_records))},
                {"crc", make_value (trailer.crc)},
                {"signature2",
                 make_value (std::begin (trailer.signature2), std::end (trailer.signature2))},
            });
        }

        value_ptr make_value (uuid const & u) { return make_value (u.str ()); }
        value_ptr make_value (index::digest const & d) {
            std::string str;
            d.to_hex (std::back_inserter (str));
            return make_value (str);
        }
        value_ptr make_value (indirect_string const & str) {
            pstore::shared_sstring_view owner;
            return make_value (str.as_db_string_view (&owner));
        }

        value_ptr make_blob (database const & db, pstore::address const begin,
                             std::uint64_t const size) {
            auto const bytes = db.getro (pstore::typed_address<std::uint8_t> (begin), size);
            return make_value (object::container{
                {"size", make_value (size)},
                {"bin", std::make_shared<binary> (bytes.get (), bytes.get () + size)},
            });
        }

    } // end namespace dump
} // end namespace pstore
