//*  _            _      _            _    *
//* | | ___   ___| | __ | |_ ___  ___| |_  *
//* | |/ _ \ / __| |/ / | __/ _ \/ __| __| *
//* | | (_) | (__|   <  | ||  __/\__ \ |_  *
//* |_|\___/ \___|_|\_\  \__\___||___/\__| *
//*                                        *
//===- tools/lock_test/lock_test.cpp --------------------------------------===//
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

#include "pstore/cmd_util/command_line.hpp"
#include "pstore/cmd_util/tchar.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/transaction.hpp"

using pstore::cmd_util::error_stream;
using pstore::cmd_util::out_stream;

namespace {

    pstore::cmd_util::cl::opt<std::string> path (pstore::cmd_util::cl::positional,
                                                 pstore::cmd_util::cl::required,
                                                 pstore::cmd_util::cl::desc ("<database-path>"));

    //*                 *
    //*  ___ __ _ _  _  *
    //* (_-</ _` | || | *
    //* /__/\__,_|\_, | *
    //*           |__/  *

    template <typename OStream, typename Arg>
    inline void say_impl (OStream & os, Arg a) {
        os << a;
    }
    template <typename OStream, typename Arg, typename... Args>
    inline void say_impl (OStream & os, Arg a, Args... args) {
        os << a;
        say_impl (os, args...);
    }


    // A wrapper around ostream operator<< which takes a lock to prevent output from different
    // threads from being interleaved.
    template <typename OStream, typename... Args>
    void say (OStream & os, Args... args) {
        static std::mutex io_mut;
        std::lock_guard<std::mutex> lock{io_mut};
        say_impl (os, args...);
        os << std::endl;
    }


    //*  _    _         _          _            _   _  __ _          *
    //* | |__| |___  __| |_____ __| |  _ _  ___| |_(_)/ _(_)___ _ _  *
    //* | '_ \ / _ \/ _| / / -_) _` | | ' \/ _ \  _| |  _| / -_) '_| *
    //* |_.__/_\___/\__|_\_\___\__,_| |_||_\___/\__|_|_| |_\___|_|   *
    //*                                                              *
    class blocked_notifier {
    public:
        blocked_notifier ();
        ~blocked_notifier ();

        // No assignment or copying.
        blocked_notifier (blocked_notifier const &) = delete;
        blocked_notifier (blocked_notifier &&) = delete;
        blocked_notifier & operator= (blocked_notifier const &) = delete;
        blocked_notifier & operator= (blocked_notifier &&) = delete;

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

    // ~dtor
    // ~~~~~
    blocked_notifier::~blocked_notifier () {
        not_blocked ();
        thread_.join ();
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

    // not_blocked
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
        pstore::cmd_util::cl::parse_command_line_options (
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
