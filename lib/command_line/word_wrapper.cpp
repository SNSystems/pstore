//===- lib/command_line/word_wrapper.cpp ----------------------------------===//
//*                        _                                             *
//* __      _____  _ __ __| | __      ___ __ __ _ _ __  _ __   ___ _ __  *
//* \ \ /\ / / _ \| '__/ _` | \ \ /\ / / '__/ _` | '_ \| '_ \ / _ \ '__| *
//*  \ V  V / (_) | | | (_| |  \ V  V /| | | (_| | |_) | |_) |  __/ |    *
//*   \_/\_/ \___/|_|  \__,_|   \_/\_/ |_|  \__,_| .__/| .__/ \___|_|    *
//*                                              |_|   |_|               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/word_wrapper.hpp"

#include <algorithm>

#include "pstore/support/assert.hpp"

namespace pstore {
    namespace command_line {

            constexpr std::size_t word_wrapper::default_width;

            // (ctor)
            // ~~~~~~
            word_wrapper::word_wrapper (std::string const & text, std::size_t const max_width,
                                        std::size_t const pos)
                    : text_{text}
                    , max_width_{max_width}
                    , start_pos_{pos} {
                this->next ();
            }

            // end
            // ~~~
            word_wrapper word_wrapper::end (std::string const & text, std::size_t const max_width) {
                return {text, max_width, text.length ()};
            }

            // operator==
            // ~~~~~~~~~~
            bool word_wrapper::operator== (word_wrapper const & rhs) const {
                return text_ == rhs.text_ && max_width_ == rhs.max_width_ &&
                       start_pos_ == rhs.start_pos_ && substr_ == rhs.substr_;
            }

            // operator++
            // ~~~~~~~~~~
            word_wrapper & word_wrapper::operator++ () {
                this->next ();
                return *this;
            }

            word_wrapper word_wrapper::operator++ (int) {
                word_wrapper prev = *this;
                ++(*this);
                return prev;
            }

            // next
            // ~~~~
            void word_wrapper::next () {
                auto const length = text_.length ();
                std::size_t end_pos = std::min (start_pos_ + max_width_, length);
                if (end_pos < length) {
                    while (end_pos > 0 && text_[end_pos] != ' ') {
                        --end_pos;
                    }
                    if (end_pos == start_pos_) {
                        // We got back to the start without finding a separator! We can't allow the
                        // next() operation to produce nothing (unless start_pos_ is at the end
                        // already). Try searching forward. This line will be too long, but that's
                        // better than looping forever.
                        while (end_pos < length && text_[end_pos] != ' ') {
                            ++end_pos;
                        }
                    }
                }

                PSTORE_ASSERT (end_pos >= start_pos_);
                substr_ = text_.substr (start_pos_, end_pos - start_pos_);

                while (end_pos < length && text_[end_pos] == ' ') {
                    ++end_pos;
                }
                start_pos_ = end_pos;
            }

    } // end namespace command_line
} // end namespace pstore
