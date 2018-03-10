//*                        _                                             *
//* __      _____  _ __ __| | __      ___ __ __ _ _ __  _ __   ___ _ __  *
//* \ \ /\ / / _ \| '__/ _` | \ \ /\ / / '__/ _` | '_ \| '_ \ / _ \ '__| *
//*  \ V  V / (_) | | | (_| |  \ V  V /| | | (_| | |_) | |_) |  __/ |    *
//*   \_/\_/ \___/|_|  \__,_|   \_/\_/ |_|  \__,_| .__/| .__/ \___|_|    *
//*                                              |_|   |_|               *
//===- lib/cmd_util/cl/word_wrapper.cpp -----------------------------------===//
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
#include "pstore/cmd_util/cl/word_wrapper.hpp"
#include <algorithm>
#include <cassert>

namespace pstore {
    namespace cmd_util {
        namespace cl {

            constexpr std::size_t word_wrapper::default_width;

            // (ctor)
            // ~~~~~~
            word_wrapper::word_wrapper (std::string const & text, std::size_t max_width,
                                        std::size_t pos)
                    : text_{text}
                    , max_width_{max_width}
                    , start_pos_{pos} {
                this->next ();
            }

            // end
            // ~~~
            word_wrapper word_wrapper::end (std::string const & text, std::size_t max_width) {
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
                std::size_t end_pos_ = std::min (start_pos_ + max_width_, length);
                if (end_pos_ < length) {
                    while (end_pos_ > 0 && text_[end_pos_] != ' ') {
                        --end_pos_;
                    }
                    if (end_pos_ == start_pos_) {
                        // We got back to the start without finding a separator! We can't allow the
                        // next() operation to produce nothing (unless start_pos_ is at the end
                        // already). Try searching forward. This line will be too long, but that's
                        // better than looping forever.
                        while (end_pos_ < length && text_[end_pos_] != ' ') {
                            ++end_pos_;
                        }
                    }
                }

                assert (end_pos_ >= start_pos_);
                substr_ = text_.substr (start_pos_, end_pos_ - start_pos_);

                while (end_pos_ < length && text_[end_pos_] == ' ') {
                    ++end_pos_;
                }
                start_pos_ = end_pos_;
            }

        } // namespace cl
    }     // namespace cmd_util
} // namespace pstore
// eof: lib/cmd_util/cl/word_wrapper.cpp
