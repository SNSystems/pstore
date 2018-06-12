//*      _ _                  _             *
//*   __| | |__   __   ____ _| |_   _  ___  *
//*  / _` | '_ \  \ \ / / _` | | | | |/ _ \ *
//* | (_| | |_) |  \ V / (_| | | |_| |  __/ *
//*  \__,_|_.__/    \_/ \__,_|_|\__,_|\___| *
//*                                         *
//===- include/pstore/dump/db_value.hpp -----------------------------------===//
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
#ifndef PSTORE_DUMP_DB_VALUE_HPP
#define PSTORE_DUMP_DB_VALUE_HPP

#include "pstore/core/database.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/dump/value.hpp"

namespace pstore {
    namespace dump {

        class address final : public value {
        public:
            explicit address (pstore::address addr)
                    : addr_{addr}
                    , expanded_{default_expanded_} {}

            bool is_number_like () const override {
                // A non-expanded address is printed just like a number.
                return !expanded_;
            }

            static bool get_expanded () noexcept { return default_expanded_; }
            static void set_expanded (bool t) noexcept { default_expanded_ = t; }

        private:
            std::ostream & write_impl (std::ostream & os, indent const & indent) const override;
            std::wostream & write_impl (std::wostream & os, indent const & indent) const override;
            value_ptr real_value () const;

            static bool default_expanded_;

            pstore::address addr_;
            bool expanded_;
            mutable value_ptr value_;
        };

        inline value_ptr make_value (pstore::address addr) {
            return std::static_pointer_cast<value> (std::make_shared<address> (addr));
        }
        template <typename T>
        inline value_ptr make_value (pstore::typed_address<T> addr) {
            return make_value (addr.to_address ());
        }

        template <typename PointerType>
        inline value_ptr make_value (sstring_view<PointerType> const & str) {
            return make_value (str.to_string ());
        }

        value_ptr make_value (pstore::header const & header);
        value_ptr make_value (pstore::trailer const & trailer, bool no_times);
        value_ptr make_value (uuid const & u);
        value_ptr make_value (index::digest const & d);
        value_ptr make_value (indirect_string const & str);

        template <typename T>
        value_ptr make_value (extent<T> ex) {
            auto v = std::make_shared<object> (object::container{
                {"addr", make_value (ex.addr)},
                {"size", make_value (ex.size)},
            });
            v->compact ();
            return v;
        }

        template <typename InputIterator>
        value_ptr make_value (InputIterator first, InputIterator last) {
            array::container members;
            std::for_each (
                first, last,
                [&members](typename std::iterator_traits<InputIterator>::value_type const & v) {
                    members.emplace_back (make_value (v));
                });
            return make_value (std::move (members));
        }

        value_ptr make_blob (database const & db, pstore::address begin, std::uint64_t size);
        value_ptr make_generation (database const & db, pstore::address footer_pos, bool no_times);
        value_ptr make_contents (database const & db,
                                 pstore::typed_address<pstore::trailer> footer_pos, bool no_times);

    } // namespace dump
} // namespace pstore

#endif // PSTORE_DUMP_DB_VALUE_HPP
// eof: include/pstore/dump/db_value.hpp
