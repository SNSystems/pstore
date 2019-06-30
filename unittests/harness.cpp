//*  _                                     *
//* | |__   __ _ _ __ _ __   ___  ___ ___  *
//* | '_ \ / _` | '__| '_ \ / _ \/ __/ __| *
//* | | | | (_| | |  | | | |  __/\__ \__ \ *
//* |_| |_|\__,_|_|  |_| |_|\___||___/___/ *
//*                                        *
//===- unittests/harness.cpp ----------------------------------------------===//
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
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <memory>

#if defined(_WIN32)
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#    if defined(_MSC_VER)
#        include <crtdbg.h>
#    endif
#endif

#include "pstore/config/config.hpp"

#if PSTORE_IS_INSIDE_LLVM
#    include "llvm/Support/Signals.h"
#endif

// (Don't be tempted to use portab.hpp to get these definitions because that would introduce a
// layering violation.)
#ifdef PSTORE_EXCEPTIONS
#    define PSTORE_TRY try
#    define PSTORE_CATCH(x, code) catch (x) code
#else
#    define PSTORE_TRY
#    define PSTORE_CATCH(x, code)
#endif

class quiet_event_listener : public testing::TestEventListener {
public:
    explicit quiet_event_listener (TestEventListener * listener);
    quiet_event_listener (quiet_event_listener const &) = delete;
    quiet_event_listener (quiet_event_listener &&) = delete;
    ~quiet_event_listener () override = default;

    quiet_event_listener & operator= (quiet_event_listener const &) = delete;
    quiet_event_listener & operator= (quiet_event_listener &&) = delete;

    void OnTestProgramStart (testing::UnitTest const & test) override;
    void OnTestIterationStart (testing::UnitTest const & test, int iteration) override;
    void OnEnvironmentsSetUpStart (testing::UnitTest const & test) override;
    void OnEnvironmentsSetUpEnd (testing::UnitTest const & test) override;
    void OnTestCaseStart (testing::TestCase const & test_case) override;
    void OnTestStart (testing::TestInfo const & test_info) override;
    void OnTestPartResult (testing::TestPartResult const & result) override;
    void OnTestEnd (testing::TestInfo const & test_info) override;
    void OnTestCaseEnd (testing::TestCase const & test_case) override;
    void OnEnvironmentsTearDownStart (testing::UnitTest const & test) override;
    void OnEnvironmentsTearDownEnd (testing::UnitTest const & test) override;
    void OnTestIterationEnd (testing::UnitTest const & test, int iteration) override;
    void OnTestProgramEnd (testing::UnitTest const & test) override;

private:
    std::unique_ptr<testing::TestEventListener> listener_;
};

quiet_event_listener::quiet_event_listener (TestEventListener * listener)
        : listener_ (listener) {}

void quiet_event_listener::OnTestProgramStart (testing::UnitTest const & test) {
    listener_->OnTestProgramStart (test);
}

void quiet_event_listener::OnTestIterationStart (testing::UnitTest const & test, int iteration) {
    listener_->OnTestIterationStart (test, iteration);
}

void quiet_event_listener::OnEnvironmentsSetUpStart (testing::UnitTest const &) {}
void quiet_event_listener::OnEnvironmentsSetUpEnd (testing::UnitTest const &) {}
void quiet_event_listener::OnEnvironmentsTearDownStart (testing::UnitTest const &) {}
void quiet_event_listener::OnEnvironmentsTearDownEnd (testing::UnitTest const &) {}

void quiet_event_listener::OnTestCaseStart (testing::TestCase const &) {}
void quiet_event_listener::OnTestCaseEnd (testing::TestCase const &) {}
void quiet_event_listener::OnTestStart (testing::TestInfo const &) {}

void quiet_event_listener::OnTestPartResult (testing::TestPartResult const & result) {
    listener_->OnTestPartResult (result);
}

void quiet_event_listener::OnTestEnd (testing::TestInfo const & test_info) {
    if (test_info.result ()->Failed ()) {
        listener_->OnTestEnd (test_info);
    }
}

void quiet_event_listener::OnTestIterationEnd (testing::UnitTest const & test, int iteration) {
    listener_->OnTestIterationEnd (test, iteration);
}

void quiet_event_listener::OnTestProgramEnd (testing::UnitTest const & test) {
    listener_->OnTestProgramEnd (test);
}


int main (int argc, char ** argv) {
    PSTORE_TRY {
#if PSTORE_IS_INSIDE_LLVM
        llvm::sys::PrintStackTraceOnErrorSignal (argv[0], true /* Disable crash reporting */);
#endif

        // Since Google Mock depends on Google Test, InitGoogleMock() is
        // also responsible for initializing Google Test. Therefore there's
        // no need for calling testing::InitGoogleTest() separately.
        testing::InitGoogleMock (&argc, argv);

        // Remove the default listener
        testing::TestEventListeners & listeners = testing::UnitTest::GetInstance ()->listeners ();
        testing::TestEventListener * const default_printer =
            listeners.Release (listeners.default_result_printer ());

        // Add our listener. By default everything is on (as when using the default listener)
        // but here we turn everything off so we only see the 3 lines for the result (plus any failures at the end), like:
        //
        // [==========] Running 149 tests from 53 test cases.
        // [==========] 149 tests from 53 test cases ran. (1 ms total)
        // [  PASSED  ] 149 tests.

        listeners.Append (new quiet_event_listener (default_printer));

#if defined(_WIN32)
        // Disable all of the possible ways Windows conspires to make automated
        // testing impossible.
        ::SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#    if defined(_MSC_VER)
        ::_set_error_mode (_OUT_TO_STDERR);
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile (_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#    endif
#endif
        return RUN_ALL_TESTS ();
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, {
        std::cerr << "Error: " << ex.what () << '\n';
    })
    PSTORE_CATCH (..., {
       std::cerr << "Unknown exception\n";
    })
    // clang-format on
    return EXIT_FAILURE;
}
