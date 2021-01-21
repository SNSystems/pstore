//*  _ _                        _ _ _   _             *
//* | (_)_ __   ___   ___ _ __ | (_) |_| |_ ___ _ __  *
//* | | | '_ \ / _ \ / __| '_ \| | | __| __/ _ \ '__| *
//* | | | | | |  __/ \__ \ |_) | | | |_| ||  __/ |    *
//* |_|_|_| |_|\___| |___/ .__/|_|_|\__|\__\___|_|    *
//*                      |_|                          *
//===- include/pstore/dump/line_splitter.hpp ------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#ifndef PSTORE_DUMP_LINE_SPLITTER_HPP
#define PSTORE_DUMP_LINE_SPLITTER_HPP

#include <algorithm>
#include <string>
#include <vector>

#include "pstore/adt/sstring_view.hpp"
#include "pstore/dump/value.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace dump {

        std::string trim_line (std::string const & str);

        template <typename InputIterator, typename OutputIterator>
        void expand_tabs (InputIterator first, InputIterator last, OutputIterator out,
                          std::size_t const tab_size = 4) {
            auto position = std::size_t{0};
            for (; first != last; ++first) {
                auto const c = *first;
                if (c != '\t') {
                    *(out++) = c;
                    ++position;
                } else {
                    auto const spaces = tab_size - (position % tab_size);
                    out = std::fill_n (out, spaces, ' ');
                    position += spaces;
                }
            }
        }


        class line_splitter {
        public:
            explicit line_splitter (gsl::not_null<dump::array::container *> const arr)
                    : arr_{arr} {}

            void append (std::string const & s) { this->append (gsl::make_span (s)); }

            void append (gsl::span<char const> const chars) {
                this->append (chars, [] (std::string const & s) { return s; });
            }
            template <typename OperationFunc>
            void append (gsl::span<char const> chars, OperationFunc operation);

        private:
            std::string str_; ///< Used to record text between calls to append()
            gsl::not_null<dump::array::container *> arr_;
        };

        template <typename OperationFunc>
        void line_splitter::append (gsl::span<char const> const chars,
                                    OperationFunc const operation) {
            PSTORE_ASSERT (chars.size () >= 0);
            sstring_view<char const *> sv (
                chars.data (), static_cast<sstring_view<char const *>::size_type> (chars.size ()));
            for (;;) {
                auto const cr_pos = sv.find ('\n', 0);
                if (cr_pos == std::string::npos) {
                    str_.append (std::begin (sv), std::end (sv));
                    break;
                }

                auto * const first = std::begin (sv);
                str_.append (first, first + cr_pos);
                arr_->emplace_back (make_value (operation (str_)));
                // Reset the accumulation buffer.
                str_.clear ();

                sv = sv.substr (cr_pos + 1U);
            }
        }

    } // namespace dump
} // namespace pstore

#endif // PSTORE_DUMP_LINE_SPLITTER_HPP
