//===- tools/lock_test/lock_test.cpp --------------------------------------===//
//*  _            _      _            _    *
//* | | ___   ___| | __ | |_ ___  ___| |_  *
//* | |/ _ \ / __| |/ / | __/ _ \/ __| __| *
//* | | (_) | (__|   <  | ||  __/\__ \ |_  *
//* |_|\___/ \___|_|\_\  \__\___||___/\__| *
//*                                        *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file lock_test.cpp
/// \brief A simple program used to test that the ptore transaction lock is working correctly.
/// \desc The transaction lock ensures that only one database instance is able to append data at any
/// given time. We do this by writing messages before the transaction, during the block (if it
/// happens), and once the transaction has been committed. Between each stage, user input is
/// required to move to the next.
///
/// We then run this program twice and step them through in sequence to ensure that one successfully
/// acquires the transaction lock and the other blocks.

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>

#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/tchar.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/transaction.hpp"

#include "say.hpp"

using pstore::command_line::error_stream;
using pstore::command_line::out_stream;

namespace {

    pstore::command_line::opt<std::string> path (pstore::command_line::positional,
                                                 pstore::command_line::required,
                                                 pstore::command_line::usage ("<repository>"));

    //*  _    _         _          _            _   _  __ _          *
    //* | |__| |___  __| |_____ __| |  _ _  ___| |_(_)/ _(_)___ _ _  *
    //* | '_ \ / _ \/ _| / / -_) _` | | ' \/ _ \  _| |  _| / -_) '_| *
    //* |_.__/_\___/\__|_\_\___\__,_| |_||_\___/\__|_|_| |_\___|_|   *
    //*                                                              *
    class blocked_notifier {
    public:
        blocked_notifier ();
        blocked_notifier (blocked_notifier const &) = delete;
        blocked_notifier (blocked_notifier &&) noexcept = delete;

        ~blocked_notifier () noexcept;

        blocked_notifier & operator= (blocked_notifier const &) = delete;
        blocked_notifier & operator= (blocked_notifier &&) noexcept = delete;

        void not_blocked ();

    private:
        /// Writes "blocked" to stdout if, after an initial delay, the process does not hold the
        /// transaction lock.
        void blocked ();

        /// The delay before we consider the process to be blocked by another holding the
        /// transaction lock.
        static constexpr auto delay_ = std::chrono::seconds{2};

        std::mutex mut_;
        std::condition_variable cv_;
        bool is_blocked_ = true;
        std::thread thread_;
    };

    constexpr std::chrono::seconds blocked_notifier::delay_;

    // ctor
    // ~~~~
    blocked_notifier::blocked_notifier ()
            : thread_{[this] () { this->blocked (); }} {}

    // dtor
    // ~~~~
    blocked_notifier::~blocked_notifier () noexcept {
        pstore::no_ex_escape ([this] () {
            not_blocked ();
            thread_.join ();
        });
    }

    // blocked
    // ~~~~~~~
    void blocked_notifier::blocked () {
        std::this_thread::sleep_for (delay_);

        std::unique_lock<std::mutex> lock{mut_};
        if (is_blocked_) {
            say (out_stream, NATIVE_TEXT ("blocked"));
            cv_.wait (lock, [this] () { return !is_blocked_; });
        }
    }

    // not blocked
    // ~~~~~~~~~~~
    void blocked_notifier::not_blocked () {
        std::lock_guard<std::mutex> _{mut_}; //! OCLint(PH - meant to be unused)
        is_blocked_ = false;
        cv_.notify_all ();
    }

} // end anonymous namespace

#ifdef _WIN32
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;
    PSTORE_TRY {
        pstore::command_line::parse_command_line_options (
            argc, argv, "pstore lock test: A simple test for the transaction lock.\n");

        say (out_stream, NATIVE_TEXT ("start"));

        std::string input;
        pstore::database db{path.get (), pstore::database::access_mode::writable};

        say (out_stream, NATIVE_TEXT ("pre-lock"));
        std::cin >> input;

        blocked_notifier notifier;
        auto transaction = pstore::begin (db);
        notifier.not_blocked ();

        say (out_stream, NATIVE_TEXT ("holding-lock"));
        std::cin >> input;

        transaction.commit ();
        say (out_stream, NATIVE_TEXT ("done"));
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, { // clang-format on
        say (error_stream, NATIVE_TEXT ("Error: "), pstore::utf::to_native_string (ex.what ()));
        exit_code = EXIT_FAILURE;
    })
    // clang-format off
    PSTORE_CATCH (..., { // clang-format on
        say (error_stream, NATIVE_TEXT ("Unknown exception.\n"));
        exit_code = EXIT_FAILURE;
    })
    return exit_code;
}
