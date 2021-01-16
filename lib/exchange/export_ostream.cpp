//*                             _               _                             *
//*   _____  ___ __   ___  _ __| |_    ___  ___| |_ _ __ ___  __ _ _ __ ___   *
//*  / _ \ \/ / '_ \ / _ \| '__| __|  / _ \/ __| __| '__/ _ \/ _` | '_ ` _ \  *
//* |  __/>  <| |_) | (_) | |  | |_  | (_) \__ \ |_| | |  __/ (_| | | | | | | *
//*  \___/_/\_\ .__/ \___/|_|   \__|  \___/|___/\__|_|  \___|\__,_|_| |_| |_| *
//*           |_|                                                             *
//===- lib/exchange/export_ostream.cpp ------------------------------------===//
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
#include "pstore/exchange/export_ostream.hpp"

#include <cinttypes>

namespace pstore {
    namespace exchange {
        namespace export_ns {

            //*         _                        _                   *
            //*  ___ __| |_ _ _ ___ __ _ _ __   | |__  __ _ ___ ___  *
            //* / _ (_-<  _| '_/ -_) _` | '  \  | '_ \/ _` (_-</ -_) *
            //* \___/__/\__|_| \___\__,_|_|_|_| |_.__/\__,_/__/\___| *
            //*                                                      *
            // ctor
            // ~~~~
            ostream_base::ostream_base (std::size_t const buffer_size) {
                buffer_.resize (buffer_size);
                ptr_ = buffer_.data ();
                end_ = ptr_ + buffer_.size ();
            }

            // dtor
            // ~~~~
            ostream_base::~ostream_base () noexcept = default;

            // write
            // ~~~~~
            ostream_base & ostream_base::write (char const c) {
                if (ptr_ == end_) {
                    this->flush ();
                }
                *(ptr_++) = c;
                return *this;
            }

            ostream_base & ostream_base::write (gsl::czstring str) {
                for (; *str != '\0'; ++str) {
                    this->write (*str);
                }
                return *this;
            }
            ostream_base & ostream_base::write (std::string const & str) {
                return this->write (gsl::make_span (str));
            }

            ostream_base & ostream_base::write (char const * s, std::streamsize const length) {
                PSTORE_ASSERT (length >= 0);
                return this->write (gsl::make_span (s, length));
            }

            // flush
            // ~~~~~
            std::size_t ostream_base::flush () {
                this->flush_buffer (buffer_, this->buffered_chars ());
                ptr_ = buffer_.data ();
                return buffer_.size ();
            }

            //*         _                       *
            //*  ___ __| |_ _ _ ___ __ _ _ __   *
            //* / _ (_-<  _| '_/ -_) _` | '  \  *
            //* \___/__/\__|_| \___\__,_|_|_|_| *
            //*                                 *
            // ctor
            // ~~~~
            ostream::ostream (FILE * const os)
                    : ostream_base (std::size_t{256 * 1024})
                    , os_ (os) {}

            // dtor
            // ~~~~
            ostream::~ostream () noexcept {
                PSTORE_TRY {
                    this->flush ();
                    // clang-format off
                } PSTORE_CATCH (..., {
                                       // clang-format on
                                   })
            }

            // flush buffer
            // ~~~~~~~~~~~~
            void ostream::flush_buffer (std::vector<char> const & buffer, std::size_t const size) {
                auto * const begin = buffer.data ();
                if (size > 0U) {
                    std::fwrite (begin, sizeof (char), size, os_);
                    if (ferror (os_)) {
                        raise (error_code::write_failed);
                    }
                }
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore
