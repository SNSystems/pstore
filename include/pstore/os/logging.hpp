//*  _                   _              *
//* | | ___   __ _  __ _(_)_ __   __ _  *
//* | |/ _ \ / _` |/ _` | | '_ \ / _` | *
//* | | (_) | (_| | (_| | | | | | (_| | *
//* |_|\___/ \__, |\__, |_|_| |_|\__, | *
//*          |___/ |___/         |___/  *
//===- include/pstore/os/logging.hpp --------------------------------------===//
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
/// \file logging.hpp

#ifndef PSTORE_OS_LOGGING_HPP
#define PSTORE_OS_LOGGING_HPP (1)

#include <cassert>
#include <ctime>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>

#include "pstore/os/file.hpp"
#include "pstore/os/thread.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/to_string.hpp"

namespace pstore {
    namespace logging {

        enum class priority {
            emergency, ///< system is unusable
            alert,     ///< action must be taken immediately
            critical,  ///< critical conditions
            error,     ///< error conditions
            warning,   ///< warning conditions
            notice,    ///< normal, but significant, condition
            info,      ///< informational message
            debug      ///< debug-level message
        };

        class quoted {
        public:
            explicit quoted (gsl::czstring str) noexcept
                    : str_{str} {}
            explicit operator gsl::czstring () const noexcept { return str_; }

        private:
            gsl::czstring str_;
        };

        //*  _                         *
        //* | |___  __ _ __ _ ___ _ _  *
        //* | / _ \/ _` / _` / -_) '_| *
        //* |_\___/\__, \__, \___|_|   *
        //*        |___/|___/          *
        /// \brief The base class for logging streams.
        class logger {
        public:
            logger () = default;
            virtual ~logger () = default;

            void set_priority (priority p) noexcept { priority_ = p; }
            priority get_priority () const noexcept { return priority_; }

            virtual void log (priority p, std::string const & message) = 0;

            virtual void log (priority p, gsl::czstring message, int d);
            virtual void log (priority p, gsl::czstring message, unsigned d);
            virtual void log (priority p, gsl::czstring message, long d);
            virtual void log (priority p, gsl::czstring message, unsigned long d);
            virtual void log (priority p, gsl::czstring message, long long d);
            virtual void log (priority p, gsl::czstring message, unsigned long long d);

            virtual void log (priority p, gsl::czstring message);
            virtual void log (priority p, gsl::czstring part1, gsl::czstring part2);
            virtual void log (priority p, gsl::czstring part1, quoted part2);

            void log (priority p, gsl::czstring message, std::string const & d) {
                this->log (p, message, d.c_str ());
            }

        private:
            template <typename T>
            static std::string to_string (char const * message, T t) {
                return message + pstore::to_string (t);
            }

            std::string buffer_;
            priority priority_ = priority::debug;
        };

        //*  _             _      _                         *
        //* | |__  __ _ __(_)__  | |___  __ _ __ _ ___ _ _  *
        //* | '_ \/ _` (_-< / _| | / _ \/ _` / _` / -_) '_| *
        //* |_.__/\__,_/__/_\__| |_\___/\__, \__, \___|_|   *
        //*                             |___/|___/          *
        class basic_logger : public logger {
        public:
            basic_logger ();

            static ::pstore::gsl::czstring priority_string (priority p);
            static std::string get_current_thread_name ();

            static constexpr std::size_t const time_buffer_size = sizeof "YYYY-MM-DDTHH:mm:SS+ZZZZ";
            static std::size_t time_string (std::time_t t,
                                            gsl::span<char, time_buffer_size> const & buffer);

            void log (priority p, std::string const & message) final;

        private:
            virtual void log_impl (std::string const & message) = 0;

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
            ~stdout_logger () noexcept override;
        };

        //*                                  *
        //*  __|_ _| _ ._._ | _  _  _  _ ._  *
        //* _> |_(_|(/_| |  |(_)(_|(_|(/_|   *
        //*                      _| _|       *
        class stderr_logger final : public file_logger {
        public:
            stderr_logger ()
                    : file_logger (stderr) {}
            ~stderr_logger () noexcept override;
        };



        struct file_system_traits {
            bool exists (std::string const & path) { return pstore::file::exists (path); }
            void rename (std::string const & from, std::string const & to) {
                pstore::file::file_handle f{from};
                f.rename (to);
            }
            void unlink (std::string const & path) { pstore::file::unlink (path.c_str ()); }
        };
        struct fstream_traits {
            using stream_type = std::ofstream;

            void open (stream_type & s, std::string const & name, std::ios_base::openmode mode) {
                s.open (name, mode);
            }
            void close (stream_type & s) { s.close (); }
            void clear (stream_type &) {}
        };


        void create_log_stream (std::string const & ident);


        namespace details {
            using logger_collection = std::vector<std::unique_ptr<logging::logger>>;
            extern PSTORE_THREAD_LOCAL logger_collection * log_destinations;
        } // end namespace details


        inline void log (priority p, gsl::czstring message) {
            if (details::log_destinations != nullptr) {
                for (std::unique_ptr<logger> & destination : *details::log_destinations) {
                    destination->log (p, message);
                }
            }
        }
        template <typename T>
        inline void log (priority p, gsl::czstring message, T d) {
            if (details::log_destinations != nullptr) {
                for (std::unique_ptr<logger> & destination : *details::log_destinations) {
                    destination->log (p, message, d);
                }
            }
        }

    } // end namespace logging
} // end namespace pstore
#endif // PSTORE_OS_LOGGING_HPP
