//*                             _               _                             *
//*   _____  ___ __   ___  _ __| |_    ___  ___| |_ _ __ ___  __ _ _ __ ___   *
//*  / _ \ \/ / '_ \ / _ \| '__| __|  / _ \/ __| __| '__/ _ \/ _` | '_ ` _ \  *
//* |  __/>  <| |_) | (_) | |  | |_  | (_) \__ \ |_| | |  __/ (_| | | | | | | *
//*  \___/_/\_\ .__/ \___/|_|   \__|  \___/|___/\__|_|  \___|\__,_|_| |_| |_| *
//*           |_|                                                             *
//===- lib/exchange/export_ostream.cpp ------------------------------------===//
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
#include "pstore/exchange/export_ostream.hpp"

#include <cinttypes>

namespace pstore {
    namespace exchange {
        namespace export_ns {

            // write
            // ~~~~~
            ostream & ostream::write (char const c) {
                std::fputc (c, os_);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
                return *this;
            }
            ostream & ostream::write (std::uint16_t const v) {
                std::fprintf (os_, "%" PRIu16, v);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
                return *this;
            }
            ostream & ostream::write (std::uint32_t const v) {
                std::fprintf (os_, "%" PRIu32, v);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
                return *this;
            }
            ostream & ostream::write (std::uint64_t const v) {
                std::fprintf (os_, "%" PRIu64, v);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
                return *this;
            }
            ostream & ostream::write (gsl::czstring const str) {
                std::fputs (str, os_);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
                return *this;
            }
            ostream & ostream::write (std::string const & str) {
                std::fputs (str.c_str (), os_);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
                return *this;
            }

            // flush
            // ~~~~~
            void ostream::flush () {
                std::fflush (os_);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore
