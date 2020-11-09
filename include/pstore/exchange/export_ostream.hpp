//*                             _               _                             *
//*   _____  ___ __   ___  _ __| |_    ___  ___| |_ _ __ ___  __ _ _ __ ___   *
//*  / _ \ \/ / '_ \ / _ \| '__| __|  / _ \/ __| __| '__/ _ \/ _` | '_ ` _ \  *
//* |  __/>  <| |_) | (_) | |  | |_  | (_) \__ \ |_| | |  __/ (_| | | | | | | *
//*  \___/_/\_\ .__/ \___/|_|   \__|  \___/|___/\__|_|  \___|\__,_|_| |_| |_| *
//*           |_|                                                             *
//===- include/pstore/exchange/export_ostream.hpp -------------------------===//
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
#ifndef PSTORE_EXCHANGE_EXPORT_OSTREAM_HPP
#define PSTORE_EXCHANGE_EXPORT_OSTREAM_HPP

#include <cstdio>
#include <string>

#include "pstore/core/indirect_string.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            class ostream {
            public:
                explicit ostream (FILE * const os)
                        : os_ (os) {}
                ostream (ostream const &) = delete;
                ostream (ostream &&) = delete;

                ~ostream () noexcept = default;

                ostream & operator= (ostream const &) = delete;
                ostream & operator= (ostream &&) = delete;

                ostream & write (char c);
                ostream & write (std::uint16_t v);
                ostream & write (std::uint32_t v);
                ostream & write (std::uint64_t v);
                ostream & write (gsl::czstring str);
                ostream & write (std::string const & str);

                template <typename T, typename = typename std::enable_if<true>::type>
                ostream & write (T const * s, std::streamsize const length) {
                    assert (length >= 0 && static_cast<std::make_unsigned_t<std::streamsize>> (
                                               length) <= std::numeric_limits<std::size_t>::max ());
                    std::fwrite (s, sizeof (T), static_cast<std::size_t> (length), os_);
                    return *this;
                }

                void flush ();

            private:
                FILE * os_;
            };

            inline ostream & operator<< (ostream & os, char const c) { return os.write (c); }
            inline ostream & operator<< (ostream & os, std::uint16_t const v) {
                return os.write (v);
            }
            inline ostream & operator<< (ostream & os, std::uint32_t const v) {
                return os.write (v);
            }
            inline ostream & operator<< (ostream & os, std::uint64_t const v) {
                return os.write (v);
            }
            inline ostream & operator<< (ostream & os, gsl::czstring const str) {
                return os.write (str);
            }
            inline ostream & operator<< (ostream & os, std::string const & str) {
                return os.write (str);
            }
            inline ostream & operator<< (ostream & os, indirect_string const & ind_str) {
                shared_sstring_view owner;
                return os << ind_str.as_string_view (&owner);
            }


            class ostream_inserter : public std::iterator<std::output_iterator_tag, char> {
            public:
                explicit ostream_inserter (ostream & os)
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
