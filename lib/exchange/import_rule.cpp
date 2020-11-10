//*  _                            _                _       *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __ _   _| | ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '__| | | | |/ _ \ *
//* | | | | | | | |_) | (_) | |  | |_  | |  | |_| | |  __/ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_|   \__,_|_|\___| *
//*             |_|                                        *
//===- lib/exchange/import_rule.cpp ---------------------------------------===//
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
/// \file import_rule.cpp
/// \brief Defines the "rule" class which models a production in the import grammar and its
/// interface with the JSON parser.

#include "pstore/exchange/import_rule.hpp"
#include "pstore/exchange/import_error.hpp"
#include "pstore/support/array_elements.hpp"

namespace {

    class indent {
    public:
        explicit constexpr indent (std::size_t const depth) noexcept
                : depth_{depth} {}
        std::ostream & write (std::ostream & os) const;

    private:
        std::size_t depth_;
    };

    std::ostream & indent::write (std::ostream & os) const {
        static char const chars[] = "  ";
        for (std::size_t ctr = 0U; ctr < depth_; ++ctr) {
            os.write (chars, pstore::array_elements (chars) - 1U);
        }
        return os;
    }

    std::ostream & operator<< (std::ostream & os, indent const & i) { return i.write (os); }

} // end anonymous namespace

namespace pstore {
    namespace exchange {
        namespace import {

            rule::~rule () = default;

            std::error_code rule::int64_value (std::int64_t const) {
                return error::unexpected_number;
            }
            std::error_code rule::uint64_value (std::uint64_t const) {
                return error::unexpected_number;
            }
            std::error_code rule::double_value (double const) { return error::unexpected_number; }
            std::error_code rule::boolean_value (bool const) { return error::unexpected_boolean; }
            std::error_code rule::null_value () { return error::unexpected_null; }
            std::error_code rule::begin_array () { return error::unexpected_array; }
            std::error_code rule::string_value (std::string const &) {
                return error::unexpected_string;
            }
            std::error_code rule::end_array () { return error::unexpected_end_array; }
            std::error_code rule::begin_object () { return error::unexpected_object; }
            std::error_code rule::key (std::string const &) { return error::unexpected_object_key; }
            std::error_code rule::end_object () { return error::unexpected_end_object; }

            void rule::log_top_impl (bool const is_push) const {
                assert (logging::enabled ());
                std::ostringstream str;
                auto const & stack = context_->stack;
                str << indent{stack.size ()} << (is_push ? '+' : '-') << stack.top ()->name ();
                logging::log (logging::priority::notice, str.str ().c_str ());
            }

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore
