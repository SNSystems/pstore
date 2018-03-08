//*  _                   _              *
//* | | ___   __ _  __ _(_)_ __   __ _  *
//* | |/ _ \ / _` |/ _` | | '_ \ / _` | *
//* | | (_) | (_| | (_| | | | | | (_| | *
//* |_|\___/ \__, |\__, |_|_| |_|\__, | *
//*          |___/ |___/         |___/  *
//===- lib/pstore_support/logging.cpp -------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
/// \file logging.cpp

#include "pstore_support/logging.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <system_error>

// pstore includes
#include "pstore/config/config.hpp"
#include "pstore_support/error.hpp"
#include "pstore_support/portab.hpp"
#include "pstore_support/rotating_log.hpp"

#if PSTORE_HAVE_ASL_H
#include <asl.h>
#include <os/log.h>
#endif
#if PSTORE_HAVE_SYS_LOG_H
#include <syslog.h>
#endif

namespace pstore {
    namespace logging {

        //*  _                         *
        //* | |___  __ _ __ _ ___ _ _  *
        //* | / _ \/ _` / _` / -_) '_| *
        //* |_\___/\__, \__, \___|_|   *
        //*        |___/|___/          *
        // log
        // ~~~
        void logger::log (priority p, char const * message, int d) {
            this->log (p, to_string (message, d));
        }
        void logger::log (priority p, char const * message, unsigned d) {
            this->log (p, to_string (message, d));
        }
        void logger::log (priority p, char const * message, long d) {
            this->log (p, to_string (message, d));
        }
        void logger::log (priority p, char const * message, unsigned long d) {
            this->log (p, to_string (message, d));
        }
        void logger::log (priority p, char const * message, long long d) {
            this->log (p, to_string (message, d));
        }
        void logger::log (priority p, char const * message, unsigned long long d) {
            this->log (p, to_string (message, d));
        }
        void logger::log (priority p, gsl::czstring message) {
            this->log (p, std::string{message});
        }
        void logger::log (priority p, gsl::czstring part1, gsl::czstring part2) {
            this->log (p, std::string{part1} + part2);
        }
        void logger::log (priority p, gsl::czstring part1, quoted part2) {
            auto message = std::string{part1};
            message += '"';
            message += part2;
            message += '"';
            this->log (p, message);
        }

//*  _             _      _                         *
//* | |__  __ _ __(_)__  | |___  __ _ __ _ ___ _ _  *
//* | '_ \/ _` (_-< / _| | / _ \/ _` / _` / -_) '_| *
//* |_.__/\__,_/__/_\__| |_\___/\__, \__, \___|_|   *
//*                             |___/|___/          *
#if PSTORE_HAVE_LOCALTIME_S
        struct tm basic_logger::local_time (time_t const & clock) {
            struct tm result;
            if (errno_t const err = localtime_s (&result, &clock)) {
                raise (errno_erc{errno}, "localtime_s");
            }
            return result;
        }
#elif PSTORE_HAVE_LOCALTIME_R
        struct tm basic_logger::local_time (time_t const & clock) {
            errno = 0;
            struct tm result {};
            struct tm * res = localtime_r (&clock, &result);
            if (res == nullptr) {
                raise (errno_erc{errno}, "localtime_r");
            }
            assert (res == &result);
            return result;
        }
#else
#error "Need localtime_r() or localtime_s()"
#endif

        // time_string
        // ~~~~~~~~~~~
        std::size_t basic_logger::time_string (std::time_t t,
                                               gsl::span<char, time_buffer_size> const & buffer) {
            static_assert (time_buffer_size > 1, "time_buffer_size is too small");

            struct tm tm_time = local_time (t);
            static constexpr char const * iso8601_format = "%FT%T%z";
            std::size_t const r =
                std::strftime (buffer.data (), time_buffer_size, iso8601_format, &tm_time);
            assert (buffer.size () >= 1);
            // guarantee NUL termination.
            buffer[buffer.size () - 1] = '\0';
            return r;
        }

    } // end namespace logging
} // end namespace pstore

namespace {

    using namespace pstore;

#if PSTORE_HAVE_ASL_H

    //*                         *
    //*  _. _| | _  _  _  _ ._  *
    //* (_|_>| |(_)(_|(_|(/_|   *
    //*             _| _|       *
    // Apple OS Log
    class asl_logger final : public logging::logger {
    public:
        explicit asl_logger (std::string const & ident);
        ~asl_logger () override;

        asl_logger (asl_logger &&) = delete;
        asl_logger (asl_logger const &) = delete;
        asl_logger & operator= (asl_logger &&) = delete;
        asl_logger & operator= (asl_logger const &) = delete;

        void log (logging::priority p, std::string const & message) override;

        void log (logging::priority p, gsl::czstring message, int d) override;
        void log (logging::priority p, gsl::czstring message, unsigned d) override;
        void log (logging::priority p, gsl::czstring message, long d) override;
        void log (logging::priority p, gsl::czstring message, unsigned long d) override;
        void log (logging::priority p, gsl::czstring message, long long d) override;
        void log (logging::priority p, gsl::czstring message, unsigned long long d) override;

        void log (logging::priority p, gsl::czstring message) override;
        void log (logging::priority p, gsl::czstring part1, gsl::czstring part2) override;
        void log (logging::priority p, gsl::czstring part1, logging::quoted part2) override;

    private:
        os_log_t log_;
        static os_log_type_t priority_code (logging::priority p);
    };

    // (ctor)
    // ~~~~~~
    asl_logger::asl_logger (std::string const & ident)
            : log_{::os_log_create (PSTORE_VENDOR_ID, ident.c_str ())} {
        if (log_ == nullptr) {
            raise (std::errc::invalid_argument, "asl_log_create");
        }
    }

    // (dtor)
    // ~~~~~~
    asl_logger::~asl_logger () { log_ = nullptr; }

    // log
    // ~~~
    void asl_logger::log (logging::priority p, std::string const & message) {
        os_log_with_type (log_, priority_code (p), "%{public}s", message.c_str ());
    }
    void asl_logger::log (logging::priority p, char const * message, int d) {
        os_log_with_type (log_, priority_code (p), "%s%d", message, d);
    }
    void asl_logger::log (logging::priority p, char const * message, unsigned d) {
        os_log_with_type (log_, priority_code (p), "%s%u", message, d);
    }
    void asl_logger::log (logging::priority p, char const * message, long d) {
        os_log_with_type (log_, priority_code (p), "%s%ld", message, d);
    }
    void asl_logger::log (logging::priority p, char const * message, unsigned long d) {
        os_log_with_type (log_, priority_code (p), "%s%lu", message, d);
    }
    void asl_logger::log (logging::priority p, char const * message, long long d) {
        os_log_with_type (log_, priority_code (p), "%s%lld", message, d);
    }
    void asl_logger::log (logging::priority p, char const * message, unsigned long long d) {
        os_log_with_type (log_, priority_code (p), "%s%llu", message, d);
    }
    void asl_logger::log (logging::priority p, gsl::czstring message) {
        os_log_with_type (log_, priority_code (p), "%s", message);
    }
    void asl_logger::log (logging::priority p, gsl::czstring part1, gsl::czstring part2) {
        os_log_with_type (log_, priority_code (p), "%s%s", part1, part2);
    }
    void asl_logger::log (logging::priority p, gsl::czstring part1, logging::quoted part2) {
        os_log_with_type (log_, priority_code (p), "%s\"%s\"", part1,
                          static_cast<gsl::czstring> (part2));
    }

    // priority_code
    // ~~~~~~~~~~~~~
    os_log_type_t asl_logger::priority_code (logging::priority p) {
        using logging::priority;
        switch (p) {
        case priority::emergency:
        case priority::alert:
        case priority::critical: return OS_LOG_TYPE_FAULT;
        case priority::error: return OS_LOG_TYPE_ERROR;
        case priority::warning: // warning conditions
        case priority::notice:  // normal, but significant, condition
        case priority::info:    // informational message
            return OS_LOG_TYPE_INFO;
        case priority::debug: // debug-level message
            return OS_LOG_TYPE_DEBUG;
        }
    }

#elif PSTORE_HAVE_SYS_LOG_H

    //*                                *
    //*  _   _| _  _  | _  _  _  _ ._  *
    //* _>\/_>|(_)(_| |(_)(_|(_|(/_|   *
    //*   /        _|      _| _|       *
    class syslog_logger final : public logging::logger {
    public:
        syslog_logger (std::string const & ident, int facility);

    private:
        void log (logging::priority p, std::string const & message) override;
        static int priority_code (logging::priority p);

        int facility_;
        char ident_[50];
    };

    // (ctor)
    // ~~~~~~
    syslog_logger::syslog_logger (std::string const & ident, int facility)
            : facility_ (facility) {

        std::strncpy (ident_, ident.c_str (), sizeof (ident_));
        ident_[sizeof (ident_) - 1] = '\0';

        openlog (ident_, LOG_PID, facility_);
    }

    // log
    // ~~~
    void syslog_logger::log (logging::priority p, std::string const & message) {
        syslog (priority_code (p), "%s", message.c_str ());
    }

    // priority_code
    // ~~~~~~~~~~~~~
    int syslog_logger::priority_code (logging::priority p) {
        switch (p) {
        case logging::priority::emergency: return LOG_EMERG;
        case logging::priority::alert: return LOG_ALERT;
        case logging::priority::critical: return LOG_CRIT;
        case logging::priority::error: return LOG_ERR;
        case logging::priority::warning: return LOG_WARNING;
        case logging::priority::notice: return LOG_NOTICE;
        case logging::priority::info: return LOG_INFO;
        case logging::priority::debug: return LOG_DEBUG;
        }
        return LOG_EMERG;
    }

#endif // PSTORE_HAVE_SYS_LOG_H

} // namespace

namespace pstore {
    namespace logging {

        namespace details {
            THREAD_LOCAL logger * log_streambuf = nullptr;
        } // namespace details

        void create_log_stream (std::unique_ptr<logging::logger> && logger) {
            using details::log_streambuf;

            delete log_streambuf;

            auto l = std::move (logger);
            log_streambuf = l.release ();
        }

        // TODO: allow user control over where the log ends up.
        void create_log_stream (std::string const & ident) {
            std::unique_ptr<logging::logger> logger;
#if PSTORE_HAVE_ASL_H
            // Redirect clog to apple system log.
            logger.reset (new asl_logger (ident));
#elif PSTORE_HAVE_SYS_LOG_H
            // Redirect clog to syslog.
            logger.reset (new syslog_logger (ident, LOG_USER));
#else

#if 0 // FIXME: temporarily disable the file-based logging. stderr makes life less bad on Windows.
            auto base_name = ident + ".log";
            constexpr auto max_size = std::ios_base::streamoff {1024 * 1024};
            constexpr auto num_backups = 10U;
            logger.reset (new logging::rotating_log (base_name, max_size, num_backups));
#else
            (void) ident;
            logger.reset (new stderr_logger);
#endif

#endif
            return create_log_stream (std::move (logger));
        }


        //*                              *
        //* |_  _. _o _ | _  _  _  _ ._  *
        //* |_)(_|_>|(_ |(_)(_|(_|(/_|   *
        //*                  _| _|       *

        std::mutex basic_logger::mutex_;

        // (ctor)
        // ~~~~~~
        basic_logger::basic_logger ()
                : thread_name_ (get_current_thread_name ()) {}

        // log
        // ~~~
        void basic_logger::log (priority p, std::string const & message) {
            std::array<char, time_buffer_size> time_buffer;
            std::size_t r = time_string (std::time (nullptr), ::gsl::make_span (time_buffer));
            assert (r == sizeof (time_buffer) - 1);
            auto time_str = time_buffer.data ();
            std::ostringstream str;
            str << time_str << " - " << thread_name_ << " - " << priority_string (p) << " - "
                << message << '\n';

            std::lock_guard<std::mutex> lock (mutex_);
            this->log_impl (str.str ());
        }

        // priority_string
        // ~~~~~~~~~~~~~~~
        gsl::czstring basic_logger::priority_string (priority p) {
            switch (p) {
            case priority::emergency: return "emergency";
            case priority::alert: return "alert";
            case priority::critical: return "critical";
            case priority::error: return "error";
            case priority::warning: return "warning";
            case priority::notice: return "notice";
            case priority::info: return "info";
            case priority::debug: return "debug";
            }
            assert (0);
            return "emergency";
        }

        // get_current_thread_name
        // ~~~~~~~~~~~~~~~~~~~~~~~
        std::string basic_logger::get_current_thread_name () {
            std::string name = threads::get_name ();
            if (name.size () == 0) {
                // If a thread name hasn't been explicitly set (or has been explictly set
                // to an empty string), then construct something that allows us to
                // identify this thread in the logs.
                std::ostringstream str;
                str << '(' << threads::get_id () << ')';
                return str.str ();
            }

            return name;
        }


        //*****************************
        //*   f i l e _ l o g g e r   *
        //*****************************
        // log_impl
        // ~~~~~~~~
        void file_logger::log_impl (std::string const & message) {
            std::fputs (message.c_str (), file_);
        }

    } // end namespace logging
} // end namespace pstore
// eof: lib/pstore_support/logging.cpp
