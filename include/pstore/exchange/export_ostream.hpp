//*                             _               _                             *
//*   _____  ___ __   ___  _ __| |_    ___  ___| |_ _ __ ___  __ _ _ __ ___   *
//*  / _ \ \/ / '_ \ / _ \| '__| __|  / _ \/ __| __| '__/ _ \/ _` | '_ ` _ \  *
//* |  __/>  <| |_) | (_) | |  | |_  | (_) \__ \ |_| | |  __/ (_| | | | | | | *
//*  \___/_/\_\ .__/ \___/|_|   \__|  \___/|___/\__|_|  \___|\__,_|_| |_| |_| *
//*           |_|                                                             *
//===- include/pstore/exchange/export_ostream.hpp -------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_EXCHANGE_EXPORT_OSTREAM_HPP
#define PSTORE_EXCHANGE_EXPORT_OSTREAM_HPP

#include <cstdio>

#include "pstore/core/indirect_string.hpp"
#include "pstore/support/unsigned_cast.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            namespace details {

                template <typename T>
                using enable_if_unsigned = typename std::enable_if_t<std::is_unsigned<T>::value>;

                /// Returns the number of characters (in base 10) that a value of type Unsigned
                /// will occupy.
                template <typename Unsigned, typename = enable_if_unsigned<Unsigned>>
                constexpr unsigned
                base10digits (Unsigned value = std::numeric_limits<Unsigned>::max ()) noexcept {
                    return value < 10U ? 1U : 1U + base10digits<Unsigned> (value / Unsigned{10});
                }

                template <typename Unsigned, typename = enable_if_unsigned<Unsigned>>
                using base10storage = std::array<char, base10digits<Unsigned> ()>;

                /// Converts an unsigned numeric value to an array of characters.
                ///
                /// \param v  The unsigned number value to be converted.
                /// \param out  A character buffer to which the output is written.
                /// \result  A pair denoting the range of valid characters in the \p out buffer.
                template <typename Unsigned, typename = enable_if_unsigned<Unsigned>>
                std::pair<char const *, char const *>
                to_characters (Unsigned v,
                               gsl::not_null<base10storage<Unsigned> *> const out) noexcept {
                    char * const end = out->data () + out->size ();
                    char * ptr = end - 1;
                    if (v == 0U) {
                        *ptr = '0';
                        return {ptr, end};
                    }

                    for (; v > 0; --ptr) {
                        *ptr = (v % 10U) + '0';
                        v /= 10U;
                    }
                    ++ptr;
                    PSTORE_ASSERT (ptr >= out->data ());
                    return {ptr, end};
                }

            } // end namespace details

            //*         _                        _                   *
            //*  ___ __| |_ _ _ ___ __ _ _ __   | |__  __ _ ___ ___  *
            //* / _ (_-<  _| '_/ -_) _` | '  \  | '_ \/ _` (_-</ -_) *
            //* \___/__/\__|_| \___\__,_|_|_|_| |_.__/\__,_/__/\___| *
            //*                                                      *
            class ostream_base {
            public:
                /// \param buffer_size  The number of characters in the output buffer.
                explicit ostream_base (std::size_t buffer_size);

                ostream_base (ostream_base const &) = delete;
                ostream_base (ostream_base &&) = delete;

                virtual ~ostream_base () noexcept = 0;

                ostream_base & operator= (ostream_base const &) = delete;
                ostream_base & operator= (ostream_base &&) = delete;

                /// Writes a single character to the output.
                ostream_base & write (char c);
                /// Writes a null-terminated string to the output.
                ostream_base & write (gsl::czstring str);
                /// Writes a string to the output.
                ostream_base & write (std::string const & str);
                ostream_base & write (char const * s, std::streamsize length);

                /// Writes an unsigned numeric value to the output.
                template <typename Unsigned,
                          typename = typename std::enable_if_t<std::is_unsigned<Unsigned>::value>>
                ostream_base & write (Unsigned v);

                /// Writes a span of characters to the output.
                template <std::ptrdiff_t Extent>
                ostream_base & write (gsl::span<char const, Extent> s);

                std::size_t flush ();

            protected:
                virtual void flush_buffer (std::vector<char> const & buffer, std::size_t size) = 0;

            private:
                /// Returns the number of characters held in the output buffer.
                std::size_t buffered_chars () const noexcept;
                /// Returns the number of characters that the buffer can accommodate before it is
                /// full.
                std::size_t available_space () const noexcept;

                std::vector<char> buffer_;
                char * ptr_ = nullptr;
                char * end_ = nullptr;
            };

            template <typename Unsigned, typename>
            ostream_base & ostream_base::write (Unsigned const v) {
                details::base10storage<Unsigned> str{{}};
                auto res = details::to_characters (v, &str);
                return this->write (gsl::make_span (res.first, res.second));
            }

            template <std::ptrdiff_t Extent>
            ostream_base & ostream_base::write (gsl::span<char const, Extent> const s) {
                if (s.empty ()) {
                    return *this;
                }
                auto index = typename gsl::span<char const, Extent>::index_type{0};
                auto remaining = unsigned_cast (s.size ());
                while (remaining > 0U) {
                    // Flush the buffer if it is full.
                    auto available = this->available_space ();
                    if (available == 0U) {
                        available = this->flush ();
                    }
                    PSTORE_ASSERT (available > 0U);

                    // Copy as many characters as we can from the input span to the buffer.
                    auto const count = std::min (remaining, available);
                    using span_index_type = gsl::span<char const>::index_type;
                    PSTORE_ASSERT (count <=
                                   unsigned_cast (std::numeric_limits<span_index_type>::max ()));
                    auto ss = s.subspan (index, static_cast<span_index_type> (count));
                    ptr_ = std::copy (ss.begin (), ss.end (), ptr_);
                    PSTORE_ASSERT (ptr_ <= end_);

                    // Adjust our view of the characters remaining to be copied.
                    remaining -= count;
                    index += count;
                }
                return *this;
            }

            inline ostream_base & operator<< (ostream_base & os, char const c) {
                return os.write (c);
            }
            inline ostream_base & operator<< (ostream_base & os, std::uint16_t const v) {
                return os.write (v);
            }
            inline ostream_base & operator<< (ostream_base & os, std::uint32_t const v) {
                return os.write (v);
            }
            inline ostream_base & operator<< (ostream_base & os, std::uint64_t const v) {
                return os.write (v);
            }
            inline ostream_base & operator<< (ostream_base & os, gsl::czstring const str) {
                return os.write (str);
            }
            inline ostream_base & operator<< (ostream_base & os, std::string const & str) {
                return os.write (str);
            }
            inline ostream_base & operator<< (ostream_base & os, indirect_string const & ind_str) {
                shared_sstring_view owner;
                return os << ind_str.as_string_view (&owner);
            }

            //*         _                       *
            //*  ___ __| |_ _ _ ___ __ _ _ __   *
            //* / _ (_-<  _| '_/ -_) _` | '  \  *
            //* \___/__/\__|_| \___\__,_|_|_|_| *
            //*                                 *
            class ostream final : public ostream_base {
            public:
                explicit ostream (FILE * os);
                ostream (ostream const &) = delete;
                ostream (ostream &&) = delete;

                ~ostream () noexcept override;

                ostream & operator= (ostream const &) = delete;
                ostream & operator= (ostream &&) = delete;

            private:
                void flush_buffer (std::vector<char> const & buffer, std::size_t size) override;

                FILE * const os_;
            };

            //*         _                        _                  _            *
            //*  ___ __| |_ _ _ ___ __ _ _ __   (_)_ _  ___ ___ _ _| |_ ___ _ _  *
            //* / _ (_-<  _| '_/ -_) _` | '  \  | | ' \(_-</ -_) '_|  _/ -_) '_| *
            //* \___/__/\__|_| \___\__,_|_|_|_| |_|_||_/__/\___|_|  \__\___|_|   *
            //*                                                                  *
            class ostream_inserter {
            public:
                using iterator_category = std::output_iterator_tag;
                using value_type = void;
                using difference_type = void;
                using pointer = void;
                using reference = void;

                explicit ostream_inserter (ostream & os) noexcept
                        : os_{os} {}
                ostream_inserter & operator= (char const c) {
                    os_ << c;
                    return *this;
                }
                ostream_inserter & operator* () noexcept { return *this; }
                ostream_inserter & operator++ () noexcept { return *this; }
                ostream_inserter operator++ (int) noexcept { return *this; }

            private:
                ostream & os_;
            };

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_OSTREAM_HPP
