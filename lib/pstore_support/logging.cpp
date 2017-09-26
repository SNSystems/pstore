//*  _                   _              *
//* | | ___   __ _  __ _(_)_ __   __ _  *
//* | |/ _ \ / _` |/ _` | | '_ \ / _` | *
//* | | (_) | (_| | (_| | | | | | (_| | *
//* |_|\___/ \__, |\__, |_|_| |_|\__, | *
//*          |___/ |___/         |___/  *
//===- lib/pstore_support/logging.cpp -------------------------------------===//
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
/// \file logging.cpp

#include "pstore_support/logging.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>

// pstore includes
#include "pstore_support/error.hpp"
#include "pstore_support/portab.hpp"
#include "pstore_support/rotating_log.hpp"
#include "support_config.hpp"

#if PSTORE_HAVE_ASL_H
#include <asl.h>
#endif
#if PSTORE_HAVE_SYS_LOG_H
#include <syslog.h>
#endif

namespace pstore {
    namespace logging {

#if PSTORE_HAVE_LOCALTIME_S

        struct tm basic_logger::local_time (time_t const & clock) {
            struct tm result;
            if (errno_t const err = localtime_s (&result, &clock)) {
                raise (errno_erc {errno}, "localtime_s");
            }
            return result;
        }

#elif PSTORE_HAVE_LOCALTIME_R

        struct tm basic_logger::local_time (time_t const & timer) {
            errno = 0;
            struct tm result {};
            struct tm * res = localtime_r (&timer, &result);
            if (res == nullptr) {
                raise (errno_erc {errno}, "localtime_r");
            }
            assert (res == &result);
            return result;
        }

#else
#error "Need localtime_r() or localtime_s()"
#endif
    } // end namespace logging
} // end namespace pstore

namespace {

#if PSTORE_HAVE_ASL_H

    //*                         *
    //*  _. _| | _  _  _  _ ._  *
    //* (_|_>| |(_)(_|(_|(/_|   *
    //*             _| _|       *
    // Apple System Logging
    class asl_logger final : public pstore::logging::logger {
    public:
        explicit asl_logger (std::string const & ident);
        ~asl_logger () override;

        asl_logger (asl_logger &&) = delete;
        asl_logger (asl_logger const &) = delete;
        asl_logger & operator= (asl_logger &&) = delete;
        asl_logger & operator= (asl_logger const &) = delete;

    private:
        void log (pstore::logging::priority p, std::string const & message) override;
        static int priority_code (pstore::logging::priority p);

        aslclient client_;
    };

    // (ctor)
    // ~~~~~~
    /// If the 'ident' string is empty, we pass nullptr to asl_open() so it will use the calling process name.
    asl_logger::asl_logger (std::string const & ident)
            : client_ (::asl_open (ident.length () > 0 ? ident.c_str () : nullptr,
                                   PSTORE_VENDOR_ID, ASL_OPT_STDERR)) {

        if (client_ == nullptr) {
            pstore::raise (std::errc::invalid_argument, "asl_open");
        }
    }

    // (dtor)
    // ~~~~~~
    asl_logger::~asl_logger () {
        ::asl_close (client_);
        client_ = nullptr;
    }

    // log
    // ~~~
    void asl_logger::log (pstore::logging::priority p, std::string const & message) {
        ::asl_log (client_, nullptr, priority_code (p), "%s", message.c_str ());
    }

    // priority_code
    // ~~~~~~~~~~~~~
    int asl_logger::priority_code (pstore::logging::priority p) {
        switch (p) {
        case pstore::logging::priority::emergency:
            return ASL_LEVEL_EMERG;
        case pstore::logging::priority::alert:
            return ASL_LEVEL_ALERT;
        case pstore::logging::priority::critical:
            return ASL_LEVEL_CRIT;
        case pstore::logging::priority::error:
            return ASL_LEVEL_ERR;
        case pstore::logging::priority::warning:
            return ASL_LEVEL_WARNING;
        case pstore::logging::priority::notice:
            return ASL_LEVEL_NOTICE;
        case pstore::logging::priority::info:
            return ASL_LEVEL_INFO;
        case pstore::logging::priority::debug:
            return ASL_LEVEL_DEBUG;
        }
        return ASL_LEVEL_EMERG;
    }

#elif PSTORE_HAVE_SYS_LOG_H

    //*                                *
    //*  _   _| _  _  | _  _  _  _ ._  *
    //* _>\/_>|(_)(_| |(_)(_|(_|(/_|   *
    //*   /        _|      _| _|       *
    class syslog_logger final : public pstore::logging::logger {
    public:
        syslog_logger (std::string const & ident, int facility);

    private:
        void log (pstore::logging::priority p, std::string const & message) override;
        static int priority_code (pstore::logging::priority p);

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
    void syslog_logger::log (pstore::logging::priority p, std::string const & message) {
        syslog (priority_code (p), "%s", message.c_str ());
    }

    // priority_code
    // ~~~~~~~~~~~~~
    int syslog_logger::priority_code (pstore::logging::priority p) {
        switch (p) {
        case pstore::logging::priority::emergency:
            return LOG_EMERG;
        case pstore::logging::priority::alert:
            return LOG_ALERT;
        case pstore::logging::priority::critical:
            return LOG_CRIT;
        case pstore::logging::priority::error:
            return LOG_ERR;
        case pstore::logging::priority::warning:
            return LOG_WARNING;
        case pstore::logging::priority::notice:
            return LOG_NOTICE;
        case pstore::logging::priority::info:
            return LOG_INFO;
        case pstore::logging::priority::debug:
            return LOG_DEBUG;
        }
        return LOG_EMERG;
    }

#endif // PSTORE_HAVE_SYS_LOG_H

} //(anonymous namespace)

namespace pstore {
    namespace logging {

        namespace details {
            THREAD_LOCAL std::ostream * log_stream = nullptr;
            THREAD_LOCAL std::streambuf * log_streambuf = nullptr;
            void sub_print (std::ostream &) {}
        } // namespace details

        std::ostream & create_log_stream (std::unique_ptr<logging::logger> && logger) {
            using details::log_stream;
            using details::log_streambuf;

            delete log_stream;
            delete log_streambuf;

            auto l = std::move (logger);
            log_streambuf = l.release ();
            log_stream = new std::ostream (log_streambuf);
            return *log_stream;
        }

        // TODO: allow user control over where the log ends up.
        std::ostream & create_log_stream (std::string const & ident) {
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


        std::ostream & operator<< (std::ostream & os, priority p) {
#ifdef PSTORE_CPP_RTTI
            if (auto * const buf = dynamic_cast<logger *> (os.rdbuf ())) {
                buf->set_priority (p);
            }
#else
            if (auto * const buf = reinterpret_cast<logger *> (os.rdbuf ())) {
                buf->set_priority (p);
            }
#endif
            return os;
        }


        //*******************
        //*   l o g g e r   *
        //*******************
        logger::logger () {}

        // overflow
        // ~~~~~~~~
        int logger::overflow (int c) {
            if (c != EOF) {
                buffer_ += static_cast<char> (c);
            } else {
                this->sync ();
            }
            return c;
        }

        // sync
        // ~~~~
        int logger::sync () {
            if (buffer_.length () > 0) {
                this->log (priority_, buffer_);
                buffer_.erase ();
                this->set_priority (priority::debug); // default to debug for each message
            }
            return 0;
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
            std::array <char, time_buffer_size> time_buffer;
            std::size_t r = time_string (std::time (nullptr), ::pstore::gsl2::make_span (time_buffer));
            assert (r == sizeof (time_buffer) - 1);
            auto time_str = time_buffer.data ();
            std::ostringstream str;
            str << time_str << " - " << thread_name_ << " - " << priority_string (p) << " - "
                << message.c_str ();

            std::lock_guard<std::mutex> lock (mutex_);
            this->log_impl (str.str ());
        }

        // priority_string
        // ~~~~~~~~~~~~~~~
        ::pstore::gsl2::czstring basic_logger::priority_string (priority p) {
            switch (p) {
            case priority::emergency:
                return "emergency";
            case priority::alert:
                return "alert";
            case priority::critical:
                return "critial";
            case priority::error:
                return "error";
            case priority::warning:
                return "warning";
            case priority::notice:
                return "notice";
            case priority::info:
                return "info";
            case priority::debug:
                return "debug";
            }
            assert (0);
            return "emergency";
        }

        // get_current_thread_name
        // ~~~~~~~~~~~~~~~~~~~~~~~
        std::string basic_logger::get_current_thread_name () {
            std::string name = pstore::threads::get_name ();
            if (name.size () == 0) {
                // If a thread name hasn't been explicitly set (or has been explictly set
                // to an empty string), then construct something that allows us to
                // identify this thread in the logs.
                std::ostringstream str;
                str << '(' << pstore::threads::get_id () << ')';
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
