//*  _ _                        _ _ _   _             *
//* | (_)_ __   ___   ___ _ __ | (_) |_| |_ ___ _ __  *
//* | | | '_ \ / _ \ / __| '_ \| | | __| __/ _ \ '__| *
//* | | | | | |  __/ \__ \ |_) | | | |_| ||  __/ |    *
//* |_|_|_| |_|\___| |___/ .__/|_|_|\__|\__\___|_|    *
//*                      |_|                          *
//===- include/dump/line_splitter.hpp -------------------------------------===//
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
#ifndef PSTORE_DUMP_LINE_SPLITTER_HPP
#define PSTORE_DUMP_LINE_SPLITTER_HPP

#include <algorithm>
#include <string>
#include <vector>
#include "dump/value.hpp"
#include "pstore_support/gsl.hpp"
#include "pstore_support/sstring_view.hpp"

namespace pstore {
    namespace dump {

        std::string trim_line (std::string const & str);

        template <typename InputIterator, typename OutputIterator>
        void expand_tabs (InputIterator first, InputIterator last, OutputIterator out,
                          std::size_t tab_size = 4) {
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
            line_splitter (gsl::not_null<dump::array::container *> arr)
                    : arr_{arr} {}

            void append (char const * s) { this->append (gsl::make_span (s, std::strlen (s))); }
            void append (std::string const & s) {
                this->append (gsl::make_span (s.data (), s.size ()));
            }

            void append (gsl::span<char const> chars) {
                this->append (chars, [](std::string const & s) { return s; });
            }
            template <typename OperationFunc>
            void append (gsl::span<char const> chars, OperationFunc operation);

        private:
            std::string str_; ///< Used to record text between calls to append()
            gsl::not_null<dump::array::container *> arr_;
        };

        template <typename OperationFunc>
        void line_splitter::append (gsl::span<char const> chars, OperationFunc operation) {
            sstring_view<char const *> sv (chars.data (), chars.size ());
            for (;;) {
                auto cr_pos = sv.find ('\n', 0);
                if (cr_pos == std::string::npos) {
                    str_.append (std::begin (sv), std::end (sv));
                    break;
                }

                auto first = std::begin (sv);
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
// eof: include/dump/line_splitter.hpp
