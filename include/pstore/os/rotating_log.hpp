//*            _        _   _               _              *
//*  _ __ ___ | |_ __ _| |_(_)_ __   __ _  | | ___   __ _  *
//* | '__/ _ \| __/ _` | __| | '_ \ / _` | | |/ _ \ / _` | *
//* | | | (_) | || (_| | |_| | | | | (_| | | | (_) | (_| | *
//* |_|  \___/ \__\__,_|\__|_|_| |_|\__, | |_|\___/ \__, | *
//*                                 |___/           |___/  *
//===- include/pstore/os/rotating_log.hpp ---------------------------------===//
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
/// \file rotating_log.hpp

#ifndef PSTORE_OS_ROTATING_LOG_HPP
#define PSTORE_OS_ROTATING_LOG_HPP

#include <iostream>
#include <limits>
#include <sstream>
#include <type_traits>

#include "pstore/os/logging.hpp"

namespace pstore {
    namespace logging {

        //*                                          *
        //* |_  _. _o _ .___|_ _._|_o._  _  | _  _   *
        //* |_)(_|_>|(_ |(_)|_(_| |_|| |(_| |(_)(_|  *
        //*                              _|      _|  *
        /// StreamTraits must:
        /// - define a type 'stream_type' which is the type of output stream, such as std::ostream
        ///   or std::ostringstream.implement the following
        /// - define the following member functions:
        ///    void open (stream_type & s, std::string const & name, std::ios_base::openmode mode);
        ///    void close (stream_type & s);
        ///    void clear (stream_type &);
        ///
        /// FileSystemTraits must define the following member functions:
        ///    bool exists (std::string const & path);
        ///    void rename (std::string const & from, std::string const & to);
        ///    void unlink (std::string const & path);

        template <typename StreamTraits, typename FileSystemTraits>
        class basic_rotating_log final : public basic_logger {
        public:
            /// \param base_name  The base file name to which an integer is appended for backup
            /// files.
            /// \param max_bytes The maximum number of bytes to which an active log file is allowed
            /// to grow before we perform a rotation and begin writing that a new file. Set to 0
            /// to allow the size to be unlimited (implying that rotation will never occur).
            /// \param num_backups The number of backup files to created and rotated. Set to 0 to
            /// cause no backups to be made.
            /// \note  Both num_backups and max_size must be greater than zero before rollover is
            /// enabled.
            basic_rotating_log (std::string base_name, std::streamoff max_bytes,
                                unsigned num_backups, StreamTraits const & traits = StreamTraits (),
                                FileSystemTraits const & fs_traits = FileSystemTraits ());
            basic_rotating_log (basic_rotating_log const &) = delete;
            basic_rotating_log (basic_rotating_log &&) = delete;

            ~basic_rotating_log () override;

            basic_rotating_log & operator= (basic_rotating_log const &) = delete;
            basic_rotating_log & operator= (basic_rotating_log &&) = delete;

            /// \brief Writes the supplied string to the log.
            /// If the current log file would become too large (as defined by the max_bytes
            /// constructor parameter), then a rotation is performed.
            void log_impl (std::string const & message) override;

            /// (for testing)
            bool is_open () const { return is_open_; }
            StreamTraits & stream_traits () { return stream_traits_; }
            FileSystemTraits & file_system_traits () { return file_system_traits_; }
            typename StreamTraits::stream_type & stream () { return stream_; }

        private:
            std::string make_file_name (unsigned index) const;
            void do_rollover ();
            void open ();
            void close ();

            /// Returns true if rollover should be performed.
            bool should_rollover (std::string const & record);

            static constexpr std::ios_base::openmode mode_flags_ =
                std::ofstream::out | std::ofstream::app | std::ofstream::ate;

            std::streamoff const max_size_;
            std::string const base_name_;
            unsigned const num_backups_;

            typename StreamTraits::stream_type stream_;
            bool is_open_ = false;
            StreamTraits stream_traits_;
            FileSystemTraits file_system_traits_;
        };

        // (ctor)
        // ~~~~~~
        template <typename StreamTraits, typename FileSystemTraits>
        basic_rotating_log<StreamTraits, FileSystemTraits>::basic_rotating_log (
            std::string base_name, std::streamoff const max_size, unsigned const num_backups,
            StreamTraits const & stream_traits, FileSystemTraits const & fs_traits)
                : max_size_ (std::max (max_size, std::streamoff{0}))
                , base_name_{std::move (base_name)}
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
        std::string basic_rotating_log<StreamTraits, FileSystemTraits>::make_file_name (
            unsigned const index) const {
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

        using rotating_log = basic_rotating_log<fstream_traits, file_system_traits>;

    } // end namespace logging
} // end namespace pstore

#endif // PSTORE_OS_ROTATING_LOG_HPP
