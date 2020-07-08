//*  _                   _              *
//* | | ___   __ _  __ _(_)_ __   __ _  *
//* | |/ _ \ / _` |/ _` | | '_ \ / _` | *
//* | | (_) | (_| | (_| | | | | | (_| | *
//* |_|\___/ \__, |\__, |_|_| |_|\__, | *
//*          |___/ |___/         |___/  *
//===- lib/os/logging.cpp -------------------------------------------------===//
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
/// \file logging.cpp

#include "pstore/os/logging.hpp"

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <system_error>

// pstore includes
#include "pstore/config/config.hpp"
#include "pstore/os/rotating_log.hpp"
#include "pstore/os/time.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/portab.hpp"

#ifdef PSTORE_HAVE_OSLOG_H
#    include <os/log.h>
#endif
#ifdef PSTORE_HAVE_SYS_LOG_H
#    include <syslog.h>
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
        void logger::log (priority const p, gsl::czstring const message, int const d) {
            this->log (p, to_string (message, d));
        }
        void logger::log (priority const p, gsl::czstring const message, unsigned const d) {
            this->log (p, to_string (message, d));
        }
        void logger::log (priority const p, gsl::czstring const message, long const d) {
            this->log (p, to_string (message, d));
        }
        void logger::log (priority const p, gsl::czstring const message, unsigned long const d) {
            this->log (p, to_string (message, d));
        }
        void logger::log (priority const p, gsl::czstring const message, long long const d) {
            this->log (p, to_string (message, d));
        }
        void logger::log (priority const p, gsl::czstring const message,
                          unsigned long long const d) {
            this->log (p, to_string (message, d));
        }
        void logger::log (priority const p, gsl::czstring const message) {
            this->log (p, std::string{message});
        }
        void logger::log (priority const p, gsl::czstring const part1, gsl::czstring const part2) {
            this->log (p, std::string{part1} + part2);
        }
        void logger::log (priority const p, gsl::czstring const part1, quoted const part2) {
            auto message = std::string{part1};
            message += '"';
            message += static_cast<gsl::czstring> (part2);
            message += '"';
            this->log (p, message);
        }

        //*  _             _      _                         *
        //* | |__  __ _ __(_)__  | |___  __ _ __ _ ___ _ _  *
        //* | '_ \/ _` (_-< / _| | / _ \/ _` / _` / -_) '_| *
        //* |_.__/\__,_/__/_\__| |_\___/\__, \__, \___|_|   *
        //*                             |___/|___/          *
        // time_string
        // ~~~~~~~~~~~
        std::size_t basic_logger::time_string (std::time_t const t,
                                               gsl::span<char, time_buffer_size> const & buffer) {
            static_assert (time_buffer_size > 1, "time_buffer_size is too small");

            struct tm const tm_time = local_time (t);
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

#ifdef PSTORE_HAVE_OSLOG_H

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
        static os_log_type_t priority_code (logging::priority p) noexcept;
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
    void asl_logger::log (logging::priority const p, std::string const & message) {
        // NOLINTNEXTLINE
        os_log_with_type (log_, priority_code (p), "%{public}s", message.c_str ()); //! OCLINT
    }
    void asl_logger::log (logging::priority const p, gsl::czstring message, int const d) {
        // NOLINTNEXTLINE
        os_log_with_type (log_, priority_code (p), "%s%d", message, d); //! OCLINT
    }
    void asl_logger::log (logging::priority const p, gsl::czstring message, unsigned const d) {
        // NOLINTNEXTLINE
        os_log_with_type (log_, priority_code (p), "%s%u", message, d); //! OCLINT
    }
    void asl_logger::log (logging::priority const p, gsl::czstring message, long const d) {
        // NOLINTNEXTLINE
        os_log_with_type (log_, priority_code (p), "%s%ld", message, d); //! OCLINT
    }
    void asl_logger::log (logging::priority const p, gsl::czstring message, unsigned long const d) {
        // NOLINTNEXTLINE
        os_log_with_type (log_, priority_code (p), "%s%lu", message, d); //! OCLINT
    }
    void asl_logger::log (logging::priority const p, gsl::czstring const message,
                          long long const d) {
        // NOLINTNEXTLINE
        os_log_with_type (log_, priority_code (p), "%s%lld", message, d); //! OCLINT
    }
    void asl_logger::log (logging::priority const p, gsl::czstring const message,
                          unsigned long long const d) {
        // NOLINTNEXTLINE
        os_log_with_type (log_, priority_code (p), "%s%llu", message, d); //! OCLINT
    }
    void asl_logger::log (logging::priority p, gsl::czstring const message) {
        // NOLINTNEXTLINE
        os_log_with_type (log_, priority_code (p), "%s", message); //! OCLINT
    }
    void asl_logger::log (logging::priority const p, gsl::czstring const part1,
                          gsl::czstring const part2) {
        // NOLINTNEXTLINE
        os_log_with_type (log_, priority_code (p), "%s%s", part1, part2); //! OCLINT
    }
    void asl_logger::log (logging::priority const p, gsl::czstring const part1,
                          logging::quoted const part2) {
        // NOLINTNEXTLINE
        os_log_with_type (log_, priority_code (p), "%s\"%s\"", part1,
                          static_cast<gsl::czstring> (part2)); //! OCLINT
    }

    // priority_code
    // ~~~~~~~~~~~~~
    os_log_type_t asl_logger::priority_code (logging::priority const p) noexcept {
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

#endif // PSTORE_HAVE_OSLOG_H

#ifdef PSTORE_HAVE_SYS_LOG_H

    //*                                *
    //*  _   _| _  _  | _  _  _  _ ._  *
    //* _>\/_>|(_)(_| |(_)(_|(_|(/_|   *
    //*   /        _|      _| _|       *
    class syslog_logger final : public logging::logger {
    public:
        syslog_logger (std::string const & ident, int facility);

    private:
        void log (logging::priority p, std::string const & message) override;
        static int priority_code (logging::priority p) noexcept;

        int facility_;
        std::array<char, 50> ident_{{0}};
    };

    // (ctor)
    // ~~~~~~
    syslog_logger::syslog_logger (std::string const & ident, int const facility)
            : facility_{facility} {

        std::strncpy (ident_.data (), ident.c_str (), sizeof (ident_));
        ident_[ident_.size () - 1] = '\0';

        openlog (ident_.data (), LOG_PID, facility_);
    }

    // log
    // ~~~
    void syslog_logger::log (logging::priority const p, std::string const & message) {
        // NOLINTNEXTLINE
        syslog (priority_code (p), "%s", message.c_str ());
    }

    // priority_code
    // ~~~~~~~~~~~~~
    int syslog_logger::priority_code (logging::priority const p) noexcept {
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

    enum handlers : unsigned {
        asl,
        rotating_file,
        standard_error,
        syslog,

        last
    };

} // namespace

namespace pstore {
    namespace logging {

        namespace details {
            PSTORE_THREAD_LOCAL logger_collection * log_destinations = nullptr;
        } // namespace details



        // TODO: allow user control over where the log ends up.
        void create_log_stream (std::string const & ident) {
            std::bitset<handlers::last> enabled;

#ifdef PSTORE_HAVE_OSLOG_H
            enabled.set (handlers::asl);
#elif defined(PSTORE_HAVE_SYS_LOG_H)
            enabled.set (handlers::syslog);
#else
            // TODO: At the moment, I just log to stderr unless syslog() or ASL are available.
            // Consider offering an option to the user to control the logging destination and
            // whether to use file-base logging.
            enabled.set (handlers::standard_error);
#endif

            auto loggers = std::make_unique<details::logger_collection> ();
            loggers->reserve (enabled.count ());

#ifdef PSTORE_HAVE_OSLOG_H
            if (enabled.test (handlers::asl)) {
                loggers->emplace_back (new asl_logger (ident));
            }
#endif
#ifdef PSTORE_HAVE_SYS_LOG_H
            if (enabled.test (handlers::syslog)) {
                // NOLINTNEXTLINE(hicpp-signed-bitwise)
                loggers->emplace_back (new syslog_logger (ident, LOG_USER));
            }
#endif

            if (enabled.test (handlers::rotating_file)) {
                constexpr auto max_size = std::streamoff{1024 * 1024};
                constexpr auto num_backups = 10U;
                loggers->emplace_back (
                    new logging::rotating_log (ident + ".log", max_size, num_backups));
            }

            if (enabled.test (handlers::standard_error)) {
                loggers->emplace_back (new stderr_logger);
            }

            using details::log_destinations;
            delete log_destinations;
            log_destinations = loggers.release ();
        }


        //*                              *
        //* |_  _. _o _ | _  _  _  _ ._  *
        //* |_)(_|_>|(_ |(_)(_|(_|(/_|   *
        //*                  _| _|       *

        std::mutex basic_logger::mutex_;

        // log
        // ~~~
        void basic_logger::log (priority const p, std::string const & message) {
            std::array<char, time_buffer_size> time_buffer;
            std::size_t const r = time_string (std::time (nullptr), ::gsl::make_span (time_buffer));
            (void) r;
            assert (r == sizeof (time_buffer) - 1);
            gsl::czstring const time_str = time_buffer.data ();
            std::ostringstream str;
            str << time_str << " - " << thread_name_ << " - " << priority_string (p) << " - "
                << message << '\n';

            std::lock_guard<std::mutex> const lock (mutex_);
            this->log_impl (str.str ());
        }

        // priority_string
        // ~~~~~~~~~~~~~~~
        gsl::czstring basic_logger::priority_string (priority const p) noexcept {
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


        stdout_logger::~stdout_logger () noexcept = default;
        stderr_logger::~stderr_logger () noexcept = default;

    } // end namespace logging
} // end namespace pstore
