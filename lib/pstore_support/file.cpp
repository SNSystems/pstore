//*   __ _ _       *
//*  / _(_) | ___  *
//* | |_| | |/ _ \ *
//* |  _| | |  __/ *
//* |_| |_|_|\___| *
//*                *
//===- lib/pstore_support/file.cpp ----------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc. 
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
/// \file file.cpp
/// \brief Definitions of the cross platform file management functions and classes.
#include "pstore_support/file.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <iterator>
#include <stdexcept>
#include <sstream>

#include "pstore_support/portab.hpp"
#include "support_config.hpp"

namespace pstore {
    namespace file {
        //*                 _                                              *
        //*   ___ _   _ ___| |_ ___ _ __ ___     ___ _ __ _ __ ___  _ __   *
        //*  / __| | | / __| __/ _ \ '_ ` _ \   / _ \ '__| '__/ _ \| '__|  *
        //*  \__ \ |_| \__ \ ||  __/ | | | | | |  __/ |  | | | (_) | |     *
        //*  |___/\__, |___/\__\___|_| |_| |_|  \___|_|  |_|  \___/|_|     *
        //*       |___/                                                    *
        // (ctor)
        // ~~~~~~
        system_error::system_error (std::error_code code, std::string const & user_message,
                                    std::string const & path)
                : std::system_error (code, message (user_message, path))
                , path_ (path) {}

        system_error::system_error (std::error_code code, char const * user_message,
                                    std::string const & path)
                : std::system_error (
                      code, message (user_message != nullptr ? user_message : "File", path))
                , path_ (path) {}

        // message
        // ~~~~~~~
        template <typename MessageStringType>
        std::string system_error::message (MessageStringType const & user_message,
                                           std::string const & path) {
            std::ostringstream stream;
            stream << user_message;
            if (path.length () > 0) {
                stream << " \"" << path << '\"';
            }
            return stream.str ();
        }


        //*                                _            _      *
        //*   _ __ __ _ _ __   __ _  ___  | | ___   ___| | __  *
        //*  | '__/ _` | '_ \ / _` |/ _ \ | |/ _ \ / __| |/ /  *
        //*  | | | (_| | | | | (_| |  __/ | | (_) | (__|   <   *
        //*  |_|  \__,_|_| |_|\__, |\___| |_|\___/ \___|_|\_\  *
        //*                   |___/                            *
        // (ctor)
        // ~~~~~~
        range_lock::range_lock () noexcept
                : range_lock (nullptr, 0U, 0U, file_base::lock_kind::shared_read) {}

        range_lock::range_lock (file_base * const file, std::uint64_t offset, std::size_t size,
                                file_base::lock_kind kind) noexcept
                : file_{file}
                , offset_{offset}
                , size_{size}
                , kind_{kind}
                , locked_{false} {}

        range_lock::range_lock (range_lock && other) noexcept
                : file_{other.file_}
                , offset_ (other.offset_)
                , size_ (other.size_)
                , kind_ (other.kind_)
                , locked_ (other.locked_) {

            other.file_ = nullptr;
            other.locked_ = false;
        }

        // (dtor)
        // ~~~~~~
        range_lock::~range_lock () noexcept {
            assert (!locked_);
        }

        // operator=
        // ~~~~~~~~~
        range_lock & range_lock::operator= (range_lock && other) {
            if (&other != this) {
                this->unlock ();
                file_ = other.file_;
                offset_ = other.offset_;
                size_ = other.size_;
                kind_ = other.kind_;
                locked_ = other.locked_;

                other.file_ = nullptr;
                other.locked_ = false;
            }
            return *this;
        }

        // lock
        // ~~~~
        bool range_lock::lock () {
            if (locked_) {
                return false;
            }
            assert (file_ != nullptr && !locked_);
            if (file_ != nullptr) {
                file_->lock (offset_, size_, kind_, file_base::blocking_mode::blocking);
                locked_ = true;
            }
            return true;
        }

        // try_lock
        // ~~~~~~~~
        bool range_lock::try_lock () {
            assert (file_ != nullptr && !locked_);
            bool result = false;
            if (file_ != nullptr) {
                result =
                    file_->lock (offset_, size_, kind_, file_base::blocking_mode::non_blocking);
                locked_ = result;
            }
            return result;
        }

        // unlock
        // ~~~~~~
        void range_lock::unlock () {
            if (locked_) {
                assert (file_ != nullptr);
                file_->unlock (offset_, size_);
                locked_ = false;
            }
        }


        //*   _                                                     *
        //*  (_)_ __    _ __ ___   ___ _ __ ___   ___  _ __ _   _   *
        //*  | | '_ \  | '_ ` _ \ / _ \ '_ ` _ \ / _ \| '__| | | |  *
        //*  | | | | | | | | | | |  __/ | | | | | (_) | |  | |_| |  *
        //*  |_|_| |_| |_| |_| |_|\___|_| |_| |_|\___/|_|   \__, |  *
        //*                                                 |___/   *
        // check_writable
        // ~~~~~~~~~~~~~~
        void in_memory::check_writable () const {
            if (!writable_) {
                raise (std::errc::permission_denied);
            }
        }

        // read_buffer
        // ~~~~~~~~~~~
        std::size_t in_memory::read_buffer (::pstore::gsl2::not_null<void *> ptr, std::size_t nbytes) {
            if (pos_ + nbytes > eof_) {
                nbytes = eof_ - pos_;
            }

            using element_type = decltype (buffer_)::element_type;
            auto span = ::pstore::gsl2::make_span (buffer_.get (), length_).subspan (pos_, nbytes);
            std::copy (std::begin (span), std::end (span),
                       static_cast<element_type *> (ptr.get ()));

            pos_ += nbytes;
            return nbytes;
        }

        // write_buffer
        // ~~~~~~~~~~~~
        void in_memory::write_buffer (::pstore::gsl2::not_null<void const *> ptr, std::size_t nbytes) {
            this->check_writable ();
            if (pos_ + nbytes > length_) {
                raise (std::errc::invalid_argument);
            }

            using element_type = decltype (buffer_)::element_type;
            auto dest_span = ::pstore::gsl2::make_span (buffer_.get (), length_).subspan (pos_, nbytes);
            auto src_span = ::pstore::gsl2::make_span (static_cast<element_type const *> (ptr.get ()), nbytes);
            std::copy (std::begin (src_span), std::end (src_span), std::begin (dest_span));

            pos_ += nbytes;
            if (pos_ > eof_) {
                eof_ = pos_;
            }
        }

        // seek
        // ~~~~
        void in_memory::seek (std::uint64_t position) {
            if (position > eof_) {
                raise (std::errc::invalid_argument);
            }

            pos_ = position;
        }

        // truncate
        // ~~~~~~~~
        void in_memory::truncate (std::uint64_t size) {
            assert (eof_ <= length_);
            assert (pos_ <= eof_);
            this->check_writable ();

            if (size > length_) {
                raise (std::errc::invalid_argument);
            }
            if (size > eof_) {
                // Fill from the current end of file to the end of the newly available region.
                std::uint8_t * const base = buffer_.get ();
                std::fill (base + eof_, base + size, std::uint8_t{0});
            }
            eof_ = size;
            // Clamp pos inside the new file extent
            pos_ = std::min (pos_, eof_);
        }

        // latest_time
        // ~~~~~~~~~~~
        std::time_t in_memory::latest_time () const {
            return 0; // FIXME: at least return the creation time.
        }


#if PSTORE_CPP_EXCEPTIONS
#define PSTORE_NO_EX_ESCAPE(x)  do { try { x; } catch (...) {} } while (0)
#else
#define PSTORE_NO_EX_ESCAPE(x)  do { x; } while (0)
#endif

        //*    __ _ _        _                     _ _        *
        //*   / _(_) | ___  | |__   __ _ _ __   __| | | ___   *
        //*  | |_| | |/ _ \ | '_ \ / _` | '_ \ / _` | |/ _ \  *
        //*  |  _| | |  __/ | | | | (_| | | | | (_| | |  __/  *
        //*  |_| |_|_|\___| |_| |_|\__,_|_| |_|\__,_|_|\___|  *
        //*                                                   *
        // (rvalue constructor)
        // ~~~~~~~~~~~~~~~~~~~~
        file_handle::file_handle (file_handle && other) noexcept
                : path_{std::move (other.path_)}
                , file_{other.file_}
                , is_writable_{other.is_writable_} {

            other.file_ = invalid_oshandle;
            other.is_writable_ = false;
        }

        // ~file_handle
        // ~~~~~~~~~~~~
        file_handle::~file_handle () noexcept {
            PSTORE_NO_EX_ESCAPE (this->close ());
        }

        // operator= (rvalue assignment)
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        file_handle & file_handle::operator= (file_handle && rhs) noexcept {
            PSTORE_NO_EX_ESCAPE (this->close ());

            path_ = std::move (rhs.path_);
            file_ = rhs.file_;
            is_writable_ = rhs.is_writable_;

            rhs.file_ = invalid_oshandle;
            rhs.is_writable_ = false;
            return *this;
        }

        std::ostream & operator<< (std::ostream & os, file_handle const & fh) {
            return os << R"({ file:")" << fh.path () << R"(" })";
        }

#undef PSTORE_NO_EX_ESCAPE

        //*       _      _      _              _                      *
        //*    __| | ___| | ___| |_ ___ _ __  | |__   __ _ ___  ___   *
        //*   / _` |/ _ \ |/ _ \ __/ _ \ '__| | '_ \ / _` / __|/ _ \  *
        //*  | (_| |  __/ |  __/ ||  __/ |    | |_) | (_| \__ \  __/  *
        //*   \__,_|\___|_|\___|\__\___|_|    |_.__/ \__,_|___/\___|  *
        //*                                                           *
        deleter_base::deleter_base (std::string path, unlink_proc unlinker)
                : path_{std::move (path)}
                , unlinker_{std::move (unlinker)} {}

        deleter_base::~deleter_base () {
            this->unlink ();
        }

        void deleter_base::unlink () {
            if (!released_) {
                unlinker_ (path_);
                released_ = true;
            }
        }
    } // end of namespace file
} // end of namespace pstore
// eof: lib/pstore_support/file.cpp
