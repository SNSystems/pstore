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
#ifndef PSTORE_HTTPD_BUFFERED_READER_HPP
#define PSTORE_HTTPD_BUFFERED_READER_HPP

#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "pstore/support/error_or.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/maybe.hpp"

namespace pstore {
    namespace httpd {
        // **************
        // * error code *
        // **************
        enum class error_code : int { string_too_long = 1, refill_out_of_range };

        // ******************
        // * error category *
        // ******************
        class error_category : public std::error_category {
        public:
            // The need for this constructor was removed by CWG defect 253 but Clang (prior
            // to 3.9.0) and GCC (before 4.6.4) require its presence.
            error_category () noexcept {} // NOLINT
            char const * name () const noexcept override;
            std::string message (int error) const override;
        };

        std::error_category const & get_error_category () noexcept;

    } // namespace httpd
} // namespace pstore


// NOLINTNEXTLINE(cert-dcl58-cpp)
namespace std {

    template <>
    struct is_error_code_enum<pstore::httpd::error_code> : std::true_type {};

    inline std::error_code make_error_code (pstore::httpd::error_code e) {
        static_assert (
            std::is_same<std::underlying_type<decltype (e)>::type, int>::value,
            "base type of pstore::httpd::error_code must be int to permit safe static cast");
        return {static_cast<int> (e), pstore::httpd::get_error_category ()};
    }

} // end namespace std

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
        /// io, char * first, char * last)`. On failure the function should return an error. On
        /// success the buffer in the range [first, last) should be populated and returning the end
        /// of data in the second half of the result pair. This value (_x_) must be (first >= x <
        /// last); end of stream is indicated by _x_ being equal to first. The updated IO value is
        /// returned in the first half of the result pair.
        template <typename IO, typename RefillFunction>
        class buffered_reader {
            using io_mchar = std::pair<IO, maybe<char>>;

        public:
            using refill_result_value = std::pair<IO, gsl::span<char>::iterator>;
            using refill_result_type = error_or<refill_result_value>;
            using getc_result_type = error_or<io_mchar>;
            using gets_result_type = error_or<std::pair<IO, maybe<std::string>>>;

            explicit buffered_reader (RefillFunction refill,
                                      std::size_t buffer_size = default_buffer_size);
            buffered_reader (buffered_reader const &) = delete;
            buffered_reader (buffered_reader &&) noexcept;

            ~buffered_reader () noexcept = default;

            buffered_reader & operator= (buffered_reader const &) = delete;
            buffered_reader & operator= (buffered_reader &&) noexcept = delete;

            /// \brief Read a single character from the data source.
            ///
            /// Returns an error or a "maybe character". The latter is just a character if one was
            /// available; it is nothing if the data source was exhausted due to an end-of-file or
            /// end-of-stream condition.
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

        private:
            void check_invariants () noexcept;
            std::ptrdiff_t pos (gsl::span<char>::iterator const & s) noexcept {
                assert (s >= span_.begin () && s <= span_.end ());
                return s - span_.begin ();
            }

            gets_result_type gets_impl (IO io, std::string const & str);

            RefillFunction refill_;
            /// The internal buffer. Filled by a call to the refill function and emptied by calls to
            /// getc().
            std::vector<char> buf_;
            /// Spans the entire contents of buf_. Both the pos_ and end_ iterators refer to this
            /// span.
            gsl::span<char> const span_;
            /// The next character in the buffer buf_.
            gsl::span<char>::iterator pos_;
            /// One beyond the last valid character in the buffer buf_.
            gsl::span<char>::iterator end_;
            /// Set to true once the refill function returns end of stream.
            bool is_eof_ = false;
            /// A one character push-back container. When just a character, getc() will yield (and
            /// reset) its value rather than extracting a character from buf_.
            maybe<char> push_;
        };

        // ctor
        // ~~~~
        template <typename IO, typename RefillFunction>
        buffered_reader<IO, RefillFunction>::buffered_reader (RefillFunction refill,
                                                              std::size_t buffer_size)
                : refill_{refill}
                , buf_ (buffer_size, char{0})
                , span_{gsl::make_span (buf_)}
                , pos_{span_.begin ()}
                , end_{span_.begin ()}
                , push_{} {
            this->check_invariants ();
        }

        template <typename IO, typename RefillFunction>
        buffered_reader<IO, RefillFunction>::buffered_reader (buffered_reader && other) noexcept
                : refill_{std::move (other.refill_)}
                , buf_{std::move (other.buf_)}
                , span_{gsl::make_span (buf_)}
                , pos_{span_.begin () + other.pos (other.pos_)}
                , end_{span_.begin () + other.pos (other.end_)}
                , is_eof_{other.is_eof_}
                , push_{std::move (other.push_)} {
            this->check_invariants ();
        }

        // getc
        // ~~~~
        template <typename IO, typename RefillFunction>
        auto buffered_reader<IO, RefillFunction>::getc (IO io) -> getc_result_type {
            // If a character has been "pushed back" then return it immediately.
            if (push_) {
                char const c = *push_;
                push_.reset ();
                return getc_result_type{in_place, io, just (c)};
            }

            // If we have characters in the buffer, return the next one.
            if (pos_ != end_) {
                this->check_invariants ();
                return getc_result_type{in_place, io, just (*(pos_++))};
            }

            // We've seen an EOF condition so don't try refilling the buffer.
            if (is_eof_) {
                return getc_result_type{in_place, io, nothing<char> ()};
            }

            auto const check_refill_response = [this](refill_result_value const & r) {
                gsl::span<char>::iterator const & end = std::get<1> (r);
                if (end < span_.begin () || end > span_.end ()) {
                    return refill_result_type{
                        std::make_error_code (error_code::refill_out_of_range)};
                }
                return refill_result_type{r};
            };

            auto const yield_result = [this](refill_result_value const & r) {
                gsl::span<char>::iterator const & end = std::get<1> (r);
                if (end == span_.begin ()) {
                    // that's the end of the source data.
                    is_eof_ = true;
                    return getc_result_type{in_place, std::get<0> (r), nothing<char> ()};
                }

                end_ = end;
                pos_ = span_.begin ();
                this->check_invariants ();
                return getc_result_type{in_place, std::get<0> (r), just (*(pos_++))};
            };

            // Refill the buffer.
            return (refill_ (io, span_) >>= check_refill_response) >>= yield_result;
        }

        // gets
        // ~~~~
        template <typename IO, typename RefillFunction>
        auto buffered_reader<IO, RefillFunction>::gets_impl (IO io, std::string const & str)
            -> gets_result_type {

            auto eos = [str](IO io2) {
                // If this is the first character of the string, then return end-of-stream. If
                // instead we hit end-of-stream after reading one or more characters, return
                // what we've got.
                return gets_result_type{in_place, io2,
                                        str.empty () ? nothing<std::string> () : just (str)};
            };

            return this->getc (io) >>= [this, &str, eos](io_mchar const & r) {
                maybe<char> const & mc = std::get<1> (r);
                if (!mc) {
                    return eos (std::get<0> (r)); // We saw end-of-stream.
                }

                constexpr auto cr = '\r';
                constexpr auto lf = '\n';
                switch (*mc) {
                case cr:
                    // We read a CR. If so, look to see if the next character is LF.
                    return this->getc (std::get<0> (r)) >>=
                           [this, str, eos, lf](io_mchar const & r2) {
                               maybe<char> const & mc2 = std::get<1> (r2);
                               if (!mc2) {
                                   return eos (std::get<0> (r2)); // We saw end-of-stream.
                               }

                               if (*mc2 != lf) {
                                   // We had a CR followed by something that's NOT an LF. Save it so
                                   // that the next call to getc() will yield it again.
                                   assert (!push_);
                                   push_ = *mc2;
                               }
                               return gets_result_type{in_place, std::get<0> (r2), just (str)};
                           };
                case lf: return gets_result_type{in_place, std::get<0> (r), just (str)};
                default: break;
                }

                if (str.length () >= max_string_length) {
                    return gets_result_type{std::make_error_code (error_code::string_too_long)};
                }
                return this->gets_impl (std::get<0> (r), str + *mc);
            };
        }

        template <typename IO, typename RefillFunction>
        auto buffered_reader<IO, RefillFunction>::gets (IO io) -> gets_result_type {
            return this->gets_impl (io, std::string{});
        }

        // check_invariants
        // ~~~~~~~~~~~~~~~~
        template <typename IO, typename RefillFunction>
        void buffered_reader<IO, RefillFunction>::check_invariants () noexcept {
#ifndef NDEBUG
            auto is_valid = [this](gsl::span<char>::iterator it) noexcept {
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

#endif // PSTORE_HTTPD_BUFFERED_READER_HPP
