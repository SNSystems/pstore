//===- lib/exchange/import_rule.cpp ---------------------------------------===//
//*  _                            _                _       *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __ _   _| | ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '__| | | | |/ _ \ *
//* | | | | | | | |_) | (_) | |  | |_  | |  | |_| | |  __/ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_|   \__,_|_|\___| *
//*             |_|                                        *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
        namespace import_ns {

            rule::~rule () = default;

            std::error_code rule::int64_value (std::int64_t const value) {
                (void) value;
                return error::unexpected_number;
            }
            std::error_code rule::uint64_value (std::uint64_t const value) {
                (void) value;
                return error::unexpected_number;
            }
            std::error_code rule::double_value (double const value) {
                (void) value;
                return error::unexpected_number;
            }
            std::error_code rule::boolean_value (bool const value) {
                (void) value;
                return error::unexpected_boolean;
            }
            std::error_code rule::null_value () { return error::unexpected_null; }
            std::error_code rule::begin_array () { return error::unexpected_array; }
            std::error_code rule::string_value (std::string const & value) {
                (void) value;
                return error::unexpected_string;
            }
            std::error_code rule::end_array () { return error::unexpected_end_array; }
            std::error_code rule::begin_object () { return error::unexpected_object; }
            std::error_code rule::key (std::string const & key) {
                (void) key;
                return error::unexpected_object_key;
            }
            std::error_code rule::end_object () { return error::unexpected_end_object; }

            void rule::log_top_impl (bool const is_push) const {
                PSTORE_ASSERT (logging_enabled ());
                std::ostringstream str;
                auto const & stack = context_->stack;
                str << indent{stack.size ()} << (is_push ? '+' : '-') << stack.top ()->name ();
                log (logger::priority::notice, str.str ().c_str ());
            }

        } // end namespace import_ns
    }     // end namespace exchange
} // end namespace pstore
