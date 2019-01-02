//*            _        _   _               _              *
//*  _ __ ___ | |_ __ _| |_(_)_ __   __ _  | | ___   __ _  *
//* | '__/ _ \| __/ _` | __| | '_ \ / _` | | |/ _ \ / _` | *
//* | | | (_) | || (_| | |_| | | | | (_| | | | (_) | (_| | *
//* |_|  \___/ \__\__,_|\__|_|_| |_|\__, | |_|\___/ \__, | *
//*                                 |___/           |___/  *
//===- include/pstore/support/rotating_log.hpp ----------------------------===//
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
/// \file rotating_log.hpp

#ifndef PSTORE_ROTATING_LOG_HPP
#define PSTORE_ROTATING_LOG_HPP (1)

#include <sstream>
#include <iostream>
#include <limits>
#include <type_traits>

#include "logging.hpp"

namespace pstore {
    namespace logging {
        //*******************************************
        //*   b a s i c _ r o t a t i n g _ l o g   *
        //*******************************************
        // (ctor)
        // ~~~~~~
        template <typename StreamTraits, typename FileSystemTraits>
        basic_rotating_log<StreamTraits, FileSystemTraits>::basic_rotating_log (
            std::string const & base_name, std::ios_base::streamoff max_size, unsigned num_backups,
            StreamTraits const & stream_traits, FileSystemTraits const & fs_traits)
                : max_size_ (std::max (max_size, std::ios_base::streamoff{0}))
                , base_name_ (base_name)
                , num_backups_ (num_backups)
                , stream_ ()
                , stream_traits_ (stream_traits)
                , file_system_traits_ (fs_traits) {}

        // (dtor)
        // ~~~~~~
        template <typename StreamTraits, typename FileSystemTraits>
        basic_rotating_log<StreamTraits, FileSystemTraits>::~basic_rotating_log () {
            this->close ();
        }

        // log
        // ~~~
        template <typename StreamTraits, typename FileSystemTraits>
        void
        basic_rotating_log<StreamTraits, FileSystemTraits>::log_impl (std::string const & message) {
            if (this->should_rollover (message)) {
                this->do_rollover ();
            }

            this->open ();
            stream_ << message;
        }

        // make_file_name
        // ~~~~~~~~~~~~~~
        template <typename StreamTraits, typename FileSystemTraits>
        std::string
        basic_rotating_log<StreamTraits, FileSystemTraits>::make_file_name (unsigned index) const {
            std::ostringstream str;
            str << base_name_;
            if (index > 0) {
                str << '.' << index;
            }
            return str.str ();
        }

        // open
        // ~~~~
        template <typename StreamTraits, typename FileSystemTraits>
        void basic_rotating_log<StreamTraits, FileSystemTraits>::open () {
            if (!is_open_) {
                stream_traits_.open (stream_, base_name_, mode_flags_);
                is_open_ = true;
            }
        }

        // close
        // ~~~~~
        template <typename StreamTraits, typename FileSystemTraits>
        void basic_rotating_log<StreamTraits, FileSystemTraits>::close () {
            if (is_open_) {
                stream_.flush ();
                stream_traits_.close (stream_);
                is_open_ = false;
            }
        }

        // do_rollover
        // ~~~~~~~~~~~
        template <typename StreamTraits, typename FileSystemTraits>
        void basic_rotating_log<StreamTraits, FileSystemTraits>::do_rollover () {
            this->close ();

            for (auto index = num_backups_; index > 0; --index) {
                std::string const & source = this->make_file_name (index - 1);
                std::string const & dest = this->make_file_name (index);
                if (file_system_traits_.exists (source)) {
                    if (file_system_traits_.exists (dest)) {
                        file_system_traits_.unlink (dest);
                    }
                    // TODO: compress the old files rather than simply rename them?
                    file_system_traits_.rename (source, dest);
                }
            }

            // Clear the stream contents. Not an issue if we're really using files (because we're
            // using a different file) but, for example, writing to a single ostringstream we'll
            // need to clear the output stream.
            stream_traits_.clear (stream_);
        }

        // should_rollover
        // ~~~~~~~~~~~~~~~
        template <typename StreamTraits, typename FileSystemTraits>
        bool basic_rotating_log<StreamTraits, FileSystemTraits>::should_rollover (
            std::string const & message) {

            bool resl = false;
            // Both num_backups_ and max_size_ must be non-zero before roll-over
            // will be enabled.
            if (stream_.good () && num_backups_ > 0 && max_size_ > 0) {
                using pos_type = typename StreamTraits::stream_type::pos_type;

                // Now we're simply trying to determine whether the file position (which will always
                // be at the end of the file) plus the length of the message will exceed
                // max_size_. Unfortunately, the way that the standard defines the result of tellp()
                // (as char_traits<char>::pos_type which is, in turn, std::streampos, and ultimately
                // std::fpos<>. This final type only supports a limited set of numeric operations.

                pos_type const pos = stream_.tellp ();
                if (pos >= 0) {
                    // Work out whether appending message would cause the current file size to
                    // exceed max_size_.
                    std::streamoff const remaining = max_size_ - pos;
                    using ustreamoff = std::make_unsigned<std::streamoff>::type;
                    if (remaining < 0 || message.length () > static_cast<ustreamoff> (remaining)) {
                        resl = true;
                    }
                }
            }
            return resl;
        }

    } // end namespace logging
} // end namespace pstore
#endif // PSTORE_ROTATING_LOG_HPP
