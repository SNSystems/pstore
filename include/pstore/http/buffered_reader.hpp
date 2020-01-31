//*  _            __  __                   _                      _            *
//* | |__  _   _ / _|/ _| ___ _ __ ___  __| |  _ __ ___  __ _  __| | ___ _ __  *
//* | '_ \| | | | |_| |_ / _ \ '__/ _ \/ _` | | '__/ _ \/ _` |/ _` |/ _ \ '__| *
//* | |_) | |_| |  _|  _|  __/ | |  __/ (_| | | | |  __/ (_| | (_| |  __/ |    *
//* |_.__/ \__,_|_| |_|  \___|_|  \___|\__,_| |_|  \___|\__,_|\__,_|\___|_|    *
//*                                                                            *
//===- include/pstore/http/buffered_reader.hpp ----------------------------===//
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
#ifndef PSTORE_HTTP_BUFFERED_READER_HPP
#define PSTORE_HTTP_BUFFERED_READER_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "pstore/http/error.hpp"
#include "pstore/support/error_or.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/maybe.hpp"


namespace pstore {
    namespace httpd {

        constexpr auto default_buffer_size = std::size_t{4096};
        constexpr auto max_string_length = std::size_t{256};

        //*  _          __  __                _                   _          *
        //* | |__ _  _ / _|/ _|___ _ _ ___ __| |  _ _ ___ __ _ __| |___ _ _  *
        //* | '_ \ || |  _|  _/ -_) '_/ -_) _` | | '_/ -_) _` / _` / -_) '_| *
        //* |_.__/\_,_|_| |_| \___|_| \___\__,_| |_| \___\__,_\__,_\___|_|   *
        //*                                                                  *
        /// \tparam IO  The type used to contain I/O state for the refill function's data source.
        ///
        /// \tparam RefillFunction  A function which is called when the buffer needs to be filled
        /// from the data source. It must have a signature compatible with `refill_result_type (IO
        /// io, gsl::span<std::uint8_t> const &)`. On failure the function should return an error.
        /// On success the buffer given by the span argument should be populated. The end of the
        /// valid bytes are denoted by the iterator returned in the second member of the result
        /// type. End of stream is indicated when this is equal to the beginning of the span
        /// argument. The updated IO value is returned in the first member of the result type.
        template <typename IO, typename RefillFunction>
        class buffered_reader {
        public:
            using state_type = IO;
            using byte_span = gsl::span<std::uint8_t>;

            using refill_result_type = error_or_n<IO, gsl::span<std::uint8_t>::iterator>;
            using geto_result_type = error_or_n<IO, maybe<std::uint8_t>>;
            using getc_result_type = error_or_n<IO, maybe<char>>;
            using gets_result_type = error_or_n<IO, maybe<std::string>>;

            explicit buffered_reader (RefillFunction refill,
                                      std::size_t buffer_size = default_buffer_size);
            buffered_reader (buffered_reader const & other) = delete;
            buffered_reader (buffered_reader && other) noexcept;

            ~buffered_reader () noexcept = default;

            buffered_reader & operator= (buffered_reader const & other) = delete;
            buffered_reader & operator= (buffered_reader && other) noexcept = delete;

            /// \brief Read a span of objects from the data source.
            ///
            /// Returns an error or a "maybe span". The latter is just span if data was available;
            /// it is nothing if the data source was exhausted due to an end-of-stream condition.
            /// Note that the returned span may be shorter than the \p sp span if insufficient data
            /// was available from the data source.
            template <typename SpanType>
            error_or_n<IO, gsl::span<typename SpanType::element_type>>
            get_span (IO io, SpanType const & sp);

            /// \brief Read a single octet from the data source.
            ///
            /// Returns an error or a "maybe byte". The latter is just a byte if one was
            /// available; it is nothing if the data source was exhausted due to an end-of-file or
            /// end-of-stream condition.
            ///
            /// \param io The IO state that will be passed to the refill function if the reader's
            /// buffer is exhausted.
            /// \returns  An error or a pair containing the (potentially updated) IO state and maybe
            /// a byte. If the latter is "nothing", this signals that the end of stream has
            /// been encountered.
            geto_result_type geto (IO io);

            /// \brief Read a single character from the data source.
            ///
            /// This is a simple wrapper for geto(): there is currently no consideration for
            /// multibyte characters.
            ///
            /// \param io The IO state that will be passed to the refill function if the reader's
            /// buffer is exhausted.
            /// \returns  An error or a pair containing the (potentially updated) IO state and maybe
            /// a character. If the latter is "nothing", this signals that the end of stream has
            /// been encountered.
            getc_result_type getc (IO io);

            /// \brief Read a string from the data source.
            ///
            /// Returns an error or a "maybe string". The latter is string if one was
            /// available; it is nothing if the data source was exhausted due to an end-of-file or
            /// end-of-stream condition. A string is sequence of characters terminated by an LF or
            /// CRLF sequence.
            ///
            /// \param io The IO state that will be passed to the refill function if the reader's
            /// buffer is exhausted.
            /// \returns  An error or a pair containing the (potentially updated) IO state and maybe
            /// a string. If the latter is "nothing", this signals that the end of stream has
            /// been encountered.
            gets_result_type gets (IO io);

            /// Returns the number of bytes that are held in the reader's buffer.
            std::size_t available () const;

        private:
            void check_invariants () noexcept;
            std::ptrdiff_t pos (gsl::span<std::uint8_t>::iterator const & s) noexcept {
                assert (s >= span_.begin () && s <= span_.end ());
                return s - span_.begin ();
            }

            gets_result_type gets_impl (IO io, std::string const & str);

            error_or_n<IO, byte_span> get_span_impl (IO io, byte_span const & sp);

            RefillFunction refill_;
            /// The internal buffer. Filled by a call to the refill function and emptied by calls to
            /// get_span_impl().
            std::vector<std::uint8_t> buf_;
            /// Spans the entire contents of buf_. Both the pos_ and end_ iterators refer to this
            /// span.
            gsl::span<std::uint8_t> const span_;
            /// The next byte in the buffer buf_.
            gsl::span<std::uint8_t>::iterator pos_;
            /// One beyond the last valid byte in the buffer buf_.
            gsl::span<std::uint8_t>::iterator end_;
            /// Set to true once the refill function returns end of stream.
            bool is_eof_ = false;
            /// A one byte push-back container. When just a byte, geto() will yield (and
            /// reset) its value rather than extracting a byte from buf_.
            maybe<std::uint8_t> push_;
        };

        // ctor
        // ~~~~
        template <typename IO, typename RefillFunction>
        buffered_reader<IO, RefillFunction>::buffered_reader (RefillFunction const refill,
                                                              std::size_t const buffer_size)
                : refill_{refill}
                , buf_ (std::max (buffer_size, std::size_t{1}), std::uint8_t{0})
                , span_{gsl::make_span (buf_)}
                , pos_{span_.begin ()}
                , end_{span_.begin ()} {
            this->check_invariants ();
        }

        template <typename IO, typename RefillFunction>
        buffered_reader<IO, RefillFunction>::buffered_reader (buffered_reader && other) noexcept
                : refill_ (std::move (other.refill_))
                , buf_{std::move (other.buf_)}
                , span_{gsl::make_span (buf_)}
                , pos_{span_.begin () + other.pos (other.pos_)}
                , end_{span_.begin () + other.pos (other.end_)}
                , is_eof_{other.is_eof_}
                , push_{std::move (other.push_)} {
            this->check_invariants ();
        }

        // get_span_impl
        // ~~~~~~~~~~~~~
        template <typename IO, typename RefillFunction>
        auto buffered_reader<IO, RefillFunction>::get_span_impl (IO io, byte_span const & sp)
            -> error_or_n<IO, byte_span> {

            auto available = sp;
            if (push_ && sp.size () > 0) {
                sp[0] = *push_;
                push_.reset ();
                available = available.subspan (1);
            }

            while (!available.empty () && !is_eof_) {
                if (pos_ == end_) {
                    // Refill the buffer.
                    error_or_n<IO, gsl::span<std::uint8_t>::iterator> x = refill_ (io, span_);
                    if (!x) {
                        return error_or_n<IO, byte_span>{x.get_error ()};
                    }
                    io = std::move (std::get<0> (x));
                    auto const & end = std::get<1> (x);
                    if (end == span_.begin ()) {
                        // that's the end of the source data.
                        is_eof_ = true;
                        return {in_place, io,
                                byte_span{sp.data (), available.data () - sp.data ()}};
                    }

                    end_ = end;
                    pos_ = span_.begin ();
                }

                // If we have bytes in the buffer, copy as many as we can.

                this->check_invariants ();
                auto to_copy = std::min (std::distance (pos_, end_), available.size ());
                std::copy_n (pos_, to_copy, available.data ());
                pos_ += to_copy;
                available = available.subspan (to_copy);
            }
            return {in_place, io, byte_span{sp.data (), available.data () - sp.data ()}};
        }

        // get_span
        // ~~~~~~~~
        template <typename IO, typename RefillFunction>
        template <typename SpanType>
        auto buffered_reader<IO, RefillFunction>::get_span (IO io, SpanType const & sp)
            -> error_or_n<IO, gsl::span<typename SpanType::element_type>> {

            auto cast = [] (IO io2, byte_span const & sp2) {
                using element_type = typename SpanType::element_type;
                using index_type = typename SpanType::index_type;
                auto const first = reinterpret_cast<element_type *> (sp2.data ());
                auto const elements =
                    std::max (sp2.size (), index_type{0}) / index_type{sizeof (element_type)};
                return error_or_n<IO, gsl::span<element_type>>{
                    in_place, io2, gsl::make_span (first, first + elements)};
            };
            return this->get_span_impl (io, as_writeable_bytes (sp)) >>= cast;
        }

        // geto
        // ~~~~
        template <typename IO, typename RefillFunction>
        auto buffered_reader<IO, RefillFunction>::geto (IO io) -> geto_result_type {
            std::uint8_t result{};
            return get_span (io, gsl::make_span (&result, 1)) >>= [] (IO io2,
                                                                      byte_span const & sp) {
                return geto_result_type{in_place, io2,
                                        sp.size () == 1 ? just (sp[0]) : nothing<std::uint8_t> ()};
            };
        }

        // getc
        // ~~~~
        template <typename IO, typename RefillFunction>
        auto buffered_reader<IO, RefillFunction>::getc (IO io) -> getc_result_type {
            // TODO: We don't consider extended characters here: only ASCII will work. Should we
            // handle UTF-8 correctly? If so, then if needs to potentially read more than one octect
            // and return a char32_t.
            return geto (io) >>= [] (IO io2, maybe<std::uint8_t> const & mb) {
                maybe<char> const mc = mb ? just (static_cast<char> (*mb)) : nothing<char> ();
                return getc_result_type{in_place, io2, mc};
            };
        }

        // gets
        // ~~~~
        template <typename IO, typename RefillFunction>
        auto buffered_reader<IO, RefillFunction>::gets_impl (IO io, std::string const & str)
            -> gets_result_type {

            auto eos = [str] (IO io2) {
                // If this is the first character of the string, then return end-of-stream. If
                // instead we hit end-of-stream after reading one or more characters, return
                // what we've got.
                return gets_result_type{in_place, io2,
                                        str.empty () ? nothing<std::string> () : just (str)};
            };

            return this->getc (io) >>= [this, &str, eos] (IO io3, maybe<char> mc) {
                if (!mc) {
                    return eos (io3); // We saw end-of-stream.
                }

                constexpr auto cr = '\r';
                constexpr auto lf = '\n';
                switch (*mc) {
                case cr:
                    // We read a CR. If so, look to see if the next character is LF.
                    return this->getc (io3) >>= [this, str, eos, lf] (IO io4, maybe<char> mc2) {
                        if (!mc2) {
                            return eos (io4); // We saw end-of-stream.
                        }

                        if (*mc2 != lf) {
                            // We had a CR followed by something that's NOT an LF. Save it so
                            // that the next call to getc() will yield it again.
                            assert (!push_);
                            // TODO: the conversion below assumes that we're only dealing
                            // with ASCII.
                            push_ = static_cast<std::uint8_t> (*mc2);
                        }
                        return gets_result_type{in_place, io4, just (str)};
                    };
                case lf: return gets_result_type{in_place, io3, just (str)};
                default: break;
                }

                if (str.length () >= max_string_length) {
                    return gets_result_type{make_error_code (error_code::string_too_long)};
                }
                return this->gets_impl (io3, str + *mc);
            };
        }

        template <typename IO, typename RefillFunction>
        auto buffered_reader<IO, RefillFunction>::gets (IO io) -> gets_result_type {
            return this->gets_impl (io, std::string{});
        }

        // available
        // ~~~~~~~~~
        template <typename IO, typename RefillFunction>
        std::size_t buffered_reader<IO, RefillFunction>::available () const {
            auto result = std::distance (pos_, end_);
            assert (result >= 0);
            if (push_) {
                ++result;
            }
            return static_cast<std::size_t> (result);
        }

        // check_invariants
        // ~~~~~~~~~~~~~~~~
        template <typename IO, typename RefillFunction>
        void buffered_reader<IO, RefillFunction>::check_invariants () noexcept {
#ifndef NDEBUG
            auto is_valid = [this] (gsl::span<std::uint8_t>::iterator it) noexcept {
                ptrdiff_t const p = this->pos (it);
                return p >= 0 &&
                       static_cast<std::make_unsigned<ptrdiff_t>::type> (p) <= buf_.size ();
            };
            assert (pos_ <= end_);
            assert (is_valid (pos_));
            assert (is_valid (end_));
#endif
        }

        // ************************
        // * make_buffered_reader *
        // ************************
        template <typename IO, typename RefillFunction>
        buffered_reader<IO, RefillFunction>
        make_buffered_reader (RefillFunction refiller,
                              std::size_t buffer_size = default_buffer_size) {
            return buffered_reader<IO, RefillFunction> (refiller, buffer_size);
        }

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTP_BUFFERED_READER_HPP
