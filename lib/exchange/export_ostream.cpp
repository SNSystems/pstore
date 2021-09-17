//===- lib/exchange/export_ostream.cpp ------------------------------------===//
//*                             _               _                             *
//*   _____  ___ __   ___  _ __| |_    ___  ___| |_ _ __ ___  __ _ _ __ ___   *
//*  / _ \ \/ / '_ \ / _ \| '__| __|  / _ \/ __| __| '__/ _ \/ _` | '_ ` _ \  *
//* |  __/>  <| |_) | (_) | |  | |_  | (_) \__ \ |_| | |  __/ (_| | | | | | | *
//*  \___/_/\_\ .__/ \___/|_|   \__|  \___/|___/\__|_|  \___|\__,_|_| |_| |_| *
//*           |_|                                                             *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
                buffer_.resize (
                    std::min (buffer_size,
                              unsigned_cast (
                                  std::numeric_limits<gsl::span<char const>::index_type>::max ())));
                ptr_ = buffer_.data ();
                end_ = ptr_ + buffer_.size ();
            }

            // dtor
            // ~~~~
            ostream_base::~ostream_base () noexcept = default;

            // write
            // ~~~~~
            ostream_base & ostream_base::write (bool const b) {
                if (b) {
                    static std::array<char, 4> const t{{'t', 'r', 'u', 'e'}};
                    return this->write (gsl::make_span (t));
                }
                static std::array<char, 5> const f{{'f', 'a', 'l', 's', 'e'}};
                return this->write (gsl::make_span (f));
            }

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

            ostream_base & ostream_base::write (char const * const s,
                                                std::streamsize const length) {
                PSTORE_ASSERT (length >= 0);
                return this->write (gsl::make_span (s, length));
            }

            // buffered chars
            // ~~~~~~~~~~~~~~
            std::size_t ostream_base::buffered_chars () const noexcept {
                auto const * const data = buffer_.data ();
                PSTORE_ASSERT (ptr_ >= data && ptr_ <= data + buffer_.size ());
                return static_cast<std::size_t> (ptr_ - data);
            }

            // available space
            // ~~~~~~~~~~~~~~~
            std::size_t ostream_base::available_space () const noexcept {
                return buffer_.size () - this->buffered_chars ();
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
                    : ostream_base (std::size_t{256} * 1024U)
                    , os_ (os) {}

            // dtor
            // ~~~~
            ostream::~ostream () noexcept {
                PSTORE_TRY {
                    this->flush ();
                    // clang-format off
                } PSTORE_CATCH (..., {})
                // clang-format on
            }

            // flush buffer
            // ~~~~~~~~~~~~
            void ostream::flush_buffer (std::vector<char> const & buffer, std::size_t const size) {
                auto const * const begin = buffer.data ();
                if (size > 0U) {
                    std::fwrite (begin, sizeof (char), size, os_);
                    if (std::ferror (os_) != 0) {
                        raise (error_code::write_failed);
                    }
                }
            }

            //*         _       _              _                       *
            //*  ___ __| |_ _ _(_)_ _  __ _ __| |_ _ _ ___ __ _ _ __   *
            //* / _ (_-<  _| '_| | ' \/ _` (_-<  _| '_/ -_) _` | '  \  *
            //* \___/__/\__|_| |_|_||_\__, /__/\__|_| \___\__,_|_|_|_| *
            //*                       |___/                            *
            std::string const & ostringstream::str () {
                this->flush ();
                return str_;
            }

            void ostringstream::flush_buffer (std::vector<char> const & buffer,
                                              std::size_t const size) {
                PSTORE_ASSERT (size < std::numeric_limits<std::string::size_type>::max ());
                str_.append (buffer.data (), size);
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore
