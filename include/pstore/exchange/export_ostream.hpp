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

#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace exchange {

        class crude_ostream {
        public:
            explicit crude_ostream (FILE * const os)
                    : os_ (os) {}
            crude_ostream (crude_ostream const &) = delete;
            crude_ostream & operator= (crude_ostream const &) = delete;

            crude_ostream & write (char c);
            crude_ostream & write (std::uint16_t v);
            crude_ostream & write (std::uint32_t v);
            crude_ostream & write (std::uint64_t v);
            crude_ostream & write (gsl::czstring str);
            crude_ostream & write (std::string const & str);

            template <typename T, typename = typename std::enable_if<true>::type>
            crude_ostream & write (T const * s, std::size_t length) {
                std::fwrite (s, sizeof (T), length, os_);
                return *this;
            }

            void flush ();

        private:
            FILE * os_;
        };

        template <typename T>
        crude_ostream & operator<< (crude_ostream & os, T const & t) {
            return os.write (t);
        }

        class ostream_inserter : public std::iterator<std::output_iterator_tag, char> {
        public:
            explicit ostream_inserter (crude_ostream & os)
                    : os_{os} {}

            ostream_inserter & operator= (char c) {
                os_ << c;
                return *this;
            }
            ostream_inserter & operator* () noexcept { return *this; }
            ostream_inserter & operator++ () noexcept { return *this; }
            ostream_inserter operator++ (int) noexcept { return *this; }

        private:
            crude_ostream & os_;
        };

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_OSTREAM_HPP
