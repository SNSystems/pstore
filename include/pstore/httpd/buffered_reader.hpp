//*  _            __  __                   _                      _            *
//* | |__  _   _ / _|/ _| ___ _ __ ___  __| |  _ __ ___  __ _  __| | ___ _ __  *
//* | '_ \| | | | |_| |_ / _ \ '__/ _ \/ _` | | '__/ _ \/ _` |/ _` |/ _ \ '__| *
//* | |_) | |_| |  _|  _|  __/ | |  __/ (_| | | | |  __/ (_| | (_| |  __/ |    *
//* |_.__/ \__,_|_| |_|  \___|_|  \___|\__,_| |_|  \___|\__,_|\__,_|\___|_|    *
//*                                                                            *
//===- include/pstore/httpd/buffered_reader.hpp ---------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_HTTPS_BUFFERED_READER_HPP
#define PSTORE_HTTPS_BUFFERED_READER_HPP

#include <array>
#include <cassert>

#include "pstore/support/error_or.hpp"
#include "pstore/support/maybe.hpp"

namespace pstore {
    namespace httpd {

        //*  _          __  __                _                   _          *
        //* | |__ _  _ / _|/ _|___ _ _ ___ __| |  _ _ ___ __ _ __| |___ _ _  *
        //* | '_ \ || |  _|  _/ -_) '_/ -_) _` | | '_/ -_) _` / _` / -_) '_| *
        //* |_.__/\_,_|_| |_| \___|_| \___\__,_| |_| \___\__,_\__,_\___|_|   *
        //*                                                                  *
        template <typename RefillFunction>
        class buffered_reader {
        public:
            explicit buffered_reader (RefillFunction refill) noexcept;
            buffered_reader (buffered_reader const &) = delete;
            buffered_reader (buffered_reader &&) noexcept;

            ~buffered_reader () noexcept = default;

            buffered_reader & operator= (buffered_reader const &) = delete;
            buffered_reader & operator= (buffered_reader &&) noexcept = delete;

            /// Read a single character from the data source.
            ///
            /// Returns an error or a "maybe character". The latter is just a character if one was
            /// available; it is nothing if the data source was exhausted due to an end-of-file or
            /// end-of-stream condition.
            error_or<maybe<char>> getc ();

            error_or<maybe<std::string>> gets ();

        private:
            void check_invariants () const noexcept;
            void check_pointer (char * s) const noexcept {
                (void) s;
                assert (s >= &buf_[0] && s < &buf_[buf_.size ()]);
            }
            std::ptrdiff_t pos (char * s) const noexcept {
                check_pointer (s);
                return s - buf_.data ();
            }

            void push_back (char c);

            RefillFunction refill_;
            std::array<char, 256> buf_;
            /// The next character in the buffer buf_.
            char * pos_;
            /// ONe beyond the last valid character in the buffer buf_.
            char * end_;
            /// Set to true once the refill function returns nullptr.
            bool is_eof_ = false;
            maybe<char> push_;
        };

        // ctor
        // ~~~~
        template <typename RefillFunction>
        buffered_reader<RefillFunction>::buffered_reader (RefillFunction refill) noexcept
                : refill_{refill}
                , buf_{{0}}
                , pos_{buf_.data ()}
                , end_{buf_.data ()}
                , push_{} {
            this->check_invariants ();
        }

        template <typename RefillFunction>
        buffered_reader<RefillFunction>::buffered_reader (buffered_reader && other) noexcept
                : refill_{std::move (other.refill_)}
                , buf_{std::move (other.buf_)}
                , pos_{buf_.data () + other.pos (other.pos_)}
                , end_{buf_.data () + other.pos (other.end_)}
                , is_eof_{other.is_eof_}
                , push_{std::move (other.push_)} {
            this->check_invariants ();
        }

        // getc
        // ~~~~
        template <typename RefillFunction>
        error_or<maybe<char>> buffered_reader<RefillFunction>::getc () {
            if (push_) {
                maybe<char> const res = push_;
                push_.reset ();
                return error_or<maybe<char>>{res};
            }

            if (pos_ == end_) {
                // We've seen an EOF condition; don't try refilling the buffer.
                if (is_eof_) {
                    return error_or<maybe<char>>{nothing<char> ()};
                }

                error_or<char *> r = refill_ (&buf_.front (), &buf_.back () + 1);
                if (r.has_error ()) {
                    return error_or<maybe<char>>{r.get_error ()};
                }

                char * const end = r.get_value ();
                if (end == nullptr || end == buf_.data ()) {
                    // that's the end of the source data.
                    is_eof_ = true;
                    return error_or<maybe<char>>{nothing<char> ()};
                }

                end_ = end;
                pos_ = buf_.data ();
            }
            this->check_invariants ();
            return error_or<maybe<char>>{just (*(pos_++))};
        }

        // gets
        // ~~~~
        template <typename RefillFunction>
        error_or<maybe<std::string>> buffered_reader<RefillFunction>::gets () {
            static auto eof = error_or<maybe<std::string>>{nothing<std::string> ()};

            auto const cr = '\r';
            auto const lf = '\n';
            std::string resl;
            for (;;) {
                auto c_err = this->getc ();
                if (c_err.has_error ()) {
                    return error_or<maybe<std::string>>{c_err.get_error ()};
                }
                maybe<char> c = c_err.get_value ();
                if (!c) {
                    // End-of-stream. If this is the first character of the string, then return
                    // end-of-stream.
                    if (resl.empty ()) {
                        return eof;
                    }
                    // We hit end-of-stream after reading one or more characters. Return what we've
                    // got.
                    break;
                }

                // Did we read a CR? If so, look to see if the next character is LF.
                if (*c == cr) {
                    c_err = this->getc ();
                    if (c_err.has_error ()) {
                        return error_or<maybe<std::string>>{c_err.get_error ()};
                    }
                    c = c_err.get_value ();
                    if (!c) {
                        // End-of-stream. If this is the first character of the string, then return
                        // end-of-stream.
                        if (resl.empty ()) {
                            return eof;
                        }
                        // We hit end-of-stream after reading one or more characters. Return what
                        // we've got.
                        break;
                    }
                    if (*c != lf) {
                        // We had a CR followed by something that's NOT an LF.
                        this->push_back (*c);
                    }
                    break;
                } else if (*c == lf) {
                    break;
                }

                resl += *c;
            }

            return error_or<maybe<std::string>>{just (resl)};
        }

        // check_invariants
        // ~~~~~~~~~~~~~~~~
        template <typename RefillFunction>
        void buffered_reader<RefillFunction>::check_invariants () const noexcept {
#ifndef NDEBUG
            char const & front = buf_.front ();
            char const & back = buf_.back ();
            assert (pos_ >= &front && pos_ <= &back + 1);
            assert (end_ >= &front && end_ <= &back + 1);
            assert (pos_ <= end_);
#endif
        }

        // push_back
        // ~~~~~~~~~
        template <typename RefillFunction>
        void buffered_reader<RefillFunction>::push_back (char c) {
            assert (!push_);
            push_ = c;
        }


        // ************************
        // * make_buffered_reader *
        // ************************
        template <typename RefillerFunction>
        buffered_reader<RefillerFunction> make_buffered_reader (RefillerFunction refiller) {
            return buffered_reader<RefillerFunction> (refiller);
        }

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTPS_BUFFERED_READER_HPP
