//*  _                   _              *
//* | | ___   __ _  __ _(_)_ __   __ _  *
//* | |/ _ \ / _` |/ _` | | '_ \ / _` | *
//* | | (_) | (_| | (_| | | | | | (_| | *
//* |_|\___/ \__, |\__, |_|_| |_|\__, | *
//*          |___/ |___/         |___/  *
//===- include/pstore_support/logging.hpp ---------------------------------===//
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
/// \file logging.hpp

#ifndef PSTORE_LOGGING_HPP
#define PSTORE_LOGGING_HPP (1)

#include <cassert>
#include <ctime>
#include <fstream>
#include <memory>
#include <mutex>
#include <streambuf>
#include <string>

#include "pstore_support/gsl.hpp"
#include "pstore_support/file.hpp"
#include "pstore_support/thread.hpp"

namespace pstore {
    namespace logging {

        enum class priority {
            emergency, // system is unusable
            alert,     // action must be taken immediately
            critical,  // critical conditions
            error,     // error conditions
            warning,   // warning conditions
            notice,    // normal, but significant, condition
            info,      // informational message
            debug      // debug-level message
        };


        //*                  *
        //* | _  _  _  _ ._  *
        //* |(_)(_|(_|(/_|   *
        //*      _| _|       *
        /// \brief The base class for logging streams.
        class logger : public std::basic_streambuf<char, std::char_traits<char>> {
        public:
            logger ();
            virtual ~logger () {}

            void set_priority (priority p) {
                priority_ = p;
            }
            priority get_priority () {
                return priority_;
            }

        protected:
            int overflow (int c) final;
            int sync () final;
            virtual void log (priority p, std::string const & message) = 0;

        private:
            std::string buffer_;
            priority priority_ = priority::debug;
        };

        //*                              *
        //* |_  _. _o _ | _  _  _  _ ._  *
        //* |_)(_|_>|(_ |(_)(_|(_|(/_|   *
        //*                  _| _|       *
        class basic_logger : public logger {
        public:
            basic_logger ();

            static ::pstore::gsl::czstring priority_string (priority p);
            static std::string get_current_thread_name ();

            static constexpr std::size_t const time_buffer_size = sizeof "YYYY-MM-DDTHH:mm:SS+ZZZZ";
            static std::size_t time_string (std::time_t t,
                                            gsl::span<char, time_buffer_size> const & buffer);

        private:
            void log (priority p, std::string const & message) final;
            virtual void log_impl (std::string const & message) = 0;

            static struct tm local_time (time_t const & clock);

            static std::mutex mutex_;
            std::string thread_name_;
        };

        //*   _                       *
        //* _|_o| _  | _  _  _  _ ._  *
        //*  | ||(/_ |(_)(_|(_|(/_|   *
        //*               _| _|       *
        class file_logger : public basic_logger {
        protected:
            explicit file_logger (FILE * file)
                    : file_{file} {}

        private:
            void log_impl (std::string const & message) override;
            FILE * file_;
        };

        //*                                   *
        //*  __|_ _| _   _|_ | _  _  _  _ ._  *
        //* _> |_(_|(_)|_||_ |(_)(_|(_|(/_|   *
        //*                       _| _|       *
        class stdout_logger final : public file_logger {
        public:
            stdout_logger ()
                    : file_logger (stdout) {}
        };

        //*                                  *
        //*  __|_ _| _ ._._ | _  _  _  _ ._  *
        //* _> |_(_|(/_| |  |(_)(_|(_|(/_|   *
        //*                      _| _|       *
        class stderr_logger final : public file_logger {
        public:
            stderr_logger ()
                    : file_logger (stderr) {}
        };



        struct file_system_traits {
            bool exists (std::string const & path) {
                return pstore::file::exists (path);
            }
            void rename (std::string const & from, std::string const & to) {
                pstore::file::rename (from.c_str (), to.c_str ());
            }
            void unlink (std::string const & path) {
                pstore::file::unlink (path.c_str ());
            }
        };
        struct fstream_traits {
            using stream_type = std::ofstream;

            void open (stream_type & s, std::string const & name, std::ios_base::openmode mode) {
                s.open (name, mode);
            }
            void close (stream_type & s) {
                s.close ();
            }
            void clear (stream_type &) {}
        };


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
            /// \param base_name The base file name to which an integer is appended for backup
            /// files.
            /// \param max_bytes The maximum number of bytes to which an active log file is allowed
            /// to
            ///                  grow before we perform a rotatation and begin writing that a new
            ///                  file. Set to 0 to allow the size to be unlimited (implying that
            ///                  rotation will never occur).
            /// \param num_backups The number of backup files to created and rotated. Set to 0 to
            /// cause
            ///                    no backups to be made.
            /// \note  Both num_backups_ and max_size_ must be greater than zero before rollover is
            ///        enabled.

            basic_rotating_log (std::string const & base_name, std::ios_base::streamoff max_bytes,
                                unsigned num_backups, StreamTraits const & traits = StreamTraits (),
                                FileSystemTraits const & fs_traits = FileSystemTraits ());
            ~basic_rotating_log () override;

            /// \brief Writes the supplied string to the log.
            /// If the current log file would become too large (as defined by the max_bytes
            /// constructor
            /// parameter), then a rotation is performed.
            void log_impl (std::string const & message) override;

            /// (for testing)
            bool is_open () const {
                return is_open_;
            }
            StreamTraits & stream_traits () {
                return stream_traits_;
            }
            FileSystemTraits & file_system_traits () {
                return file_system_traits_;
            }
            typename StreamTraits::stream_type & stream () {
                return stream_;
            }

        private:
            std::string make_file_name (unsigned index) const;
            void do_rollover ();
            void open ();
            void close ();

            /// Returns true if rollover should be performed.
            bool should_rollover (std::string const & record);

            static constexpr std::ios_base::openmode mode_flags_ =
                std::ofstream::out | std::ofstream::app | std::ofstream::ate;

            std::ios_base::streamoff const max_size_;
            std::string const base_name_;
            unsigned const num_backups_;

            typename StreamTraits::stream_type stream_;
            bool is_open_ = false;
            StreamTraits stream_traits_;
            FileSystemTraits file_system_traits_;
        };

        using rotating_log = basic_rotating_log<fstream_traits, file_system_traits>;



        std::ostream & operator<< (std::ostream & os, priority p);


        std::ostream & create_log_stream (std::unique_ptr<logger> && logger);
        std::ostream & create_log_stream (std::string const & ident);


        namespace details {
            extern THREAD_LOCAL std::ostream * log_stream;
            extern THREAD_LOCAL std::streambuf * log_streambuf;

            void sub_print (std::ostream &);
            template <class A0, class... Args>
            void sub_print (std::ostream & os, A0 const & a0, Args const &... args) {
                os << a0;
                sub_print (os, args...);
            }
        } // end namespace details

        template <class... Args>
        void log (priority p, Args const &... args) noexcept {
            assert (details::log_stream != nullptr);
            auto & os = *details::log_stream;
            os << p;
            details::sub_print (os, args...);
            os << std::endl;
        }

    } // end namespace logging
} // end namespace pstore
#endif // PSTORE_LOGGING_HPP
// eof: include/pstore_support/logging.hpp
