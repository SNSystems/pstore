//*                                  *
//*  ___ _ __   __ ___      ___ __   *
//* / __| '_ \ / _` \ \ /\ / / '_ \  *
//* \__ \ |_) | (_| |\ V  V /| | | | *
//* |___/ .__/ \__,_| \_/\_/ |_| |_| *
//*     |_|                          *
//===- include/pstore/broker/spawn.hpp ------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file spawn.hpp

#ifndef PSTORE_BROKER_SPAWN_HPP
#define PSTORE_BROKER_SPAWN_HPP

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <type_traits>

// platform includes
#ifdef _WIN32
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#else
#    include <sys/types.h>
#endif // _WIN32

#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace broker {

#ifdef _WIN32
        namespace win32 {

            /// This routine appends the given argument to a command line such
            /// that CommandLineToArgvW will return the argument string unchanged.
            /// Arguments in a command line should be separated by spaces; this
            /// function does not add these spaces.
            ///
            /// Based on code published in an MSDN blog article titled "Everyone
            /// quotes command line arguments the wrong way" (Daniel Colascione,
            /// April 23 2011).
            ///
            /// \note This function is exposed to enable it to be unit tested.
            ///
            /// \param arg  The argument to encode.
            /// \param force  Supplies an indication of whether we should quote the argument even if
            /// it does not contain any characters that would ordinarily require quoting.
            /// \return  The quoted argument string.
            std::string argv_quote (std::string const & arg, bool force = false);

            /// Takes an array of command-line argument strings and converts them
            /// to a single string
            ///
            /// \note This function is exposed to enable it to be unit tested.
            std::string build_command_line (gsl::not_null<gsl::czstring const *> argv);

            class process_pair {
            public:
                process_pair (HANDLE p, DWORD g) noexcept;
                ~process_pair () noexcept = default;

                process_pair (process_pair const &) = delete;
                process_pair (process_pair &&) noexcept = delete;
                process_pair & operator= (process_pair const &) = delete;
                process_pair & operator= (process_pair &&) noexcept = delete;

                bool operator== (process_pair const & other) const noexcept {
                    return process_.get () == other.process_.get () && group_ == other.group_;
                }
                bool operator!= (process_pair const & other) const noexcept {
                    return !operator== (other);
                }

                HANDLE process () const noexcept { return process_.get (); }
                DWORD group () const noexcept { return group_; }

            private:
                static void close_handle (HANDLE h) noexcept;

                std::unique_ptr<std::remove_pointer<HANDLE>::type, decltype (&close_handle)> const
                    process_;
                DWORD const group_;
            };

        } // end namespace win32
#endif

#ifdef _WIN32
        using process_identifier = std::shared_ptr<win32::process_pair>;
#else
        using process_identifier = pid_t;
#endif

        /// Starts the process at the location given by `exe_path` and with the arguments
        /// supplied in a null-terminated array of czstrings.
        process_identifier spawn (gsl::czstring exe_path,
                                  gsl::not_null<gsl::czstring const *> argv);

        inline process_identifier spawn (std::initializer_list<gsl::czstring> argv) {
            // There must be at least two entries (the process path and terminating null).
            PSTORE_ASSERT (argv.size () >= 2);
            gsl::czstring const * const begin = std::begin (argv);
            // The last array element must be null.
            PSTORE_ASSERT (*(begin + argv.size () - 1) == nullptr);
            return spawn (*begin, begin);
        }

    } // namespace broker
} // namespace pstore

#endif // PSTORE_BROKER_SPAWN_HPP
