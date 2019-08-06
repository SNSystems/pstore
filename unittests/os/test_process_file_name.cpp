//*                                       __ _ _                                    *
//*  _ __  _ __ ___   ___ ___  ___ ___   / _(_) | ___   _ __   __ _ _ __ ___   ___  *
//* | '_ \| '__/ _ \ / __/ _ \/ __/ __| | |_| | |/ _ \ | '_ \ / _` | '_ ` _ \ / _ \ *
//* | |_) | | | (_) | (_|  __/\__ \__ \ |  _| | |  __/ | | | | (_| | | | | | |  __/ *
//* | .__/|_|  \___/ \___\___||___/___/ |_| |_|_|\___| |_| |_|\__,_|_| |_| |_|\___| *
//* |_|                                                                             *
//===- unittests/os/test_process_file_name.cpp ----------------------------===//
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

/// \file test_process_file_name.cpp

#include "pstore/os/process_file_name.hpp"

#include <algorithm>
#include <cstring>
#include <functional>
// 3rd party
#include "gmock/gmock.h"

#include "pstore/support/gsl.hpp"
#include "pstore/support/small_vector.hpp"
#include "check_for_error.hpp"

namespace {
    template <typename T>
    typename std::make_unsigned<T>::type as_unsigned (T x) {
        using utype = typename std::make_unsigned<T>::type;
        assert (x >= 0);
        return static_cast<utype> (x);
    }

    class ProcessFileName : public ::testing::Test {
    protected:
        static auto get_process_path (::pstore::gsl::span<char> b) -> std::size_t;
    };

    auto ProcessFileName::get_process_path (::pstore::gsl::span<char> b) -> std::size_t {
        char const result[] = "process path";
        auto num_to_copy = std::min (static_cast<std::ptrdiff_t> (sizeof (result) - 1), b.size ());
        auto begin = std::begin (b);
        auto out = std::copy_n (result, num_to_copy, begin);
        return as_unsigned (std::distance (begin, out));
    }
} // namespace

TEST_F (ProcessFileName, BufferContents) {
    std::vector<char> buffer;
    std::size_t length = pstore::process_file_name (&get_process_path, buffer);
    EXPECT_EQ (length, 12U);
    buffer.resize (12U); // lose any additional buffer bytes.
    std::vector<char> expected{'p', 'r', 'o', 'c', 'e', 's', 's', ' ', 'p', 'a', 't', 'h'};
    EXPECT_THAT (buffer, ::testing::ContainerEq (expected));
}

TEST_F (ProcessFileName, BufferContentsWithInitialSize) {
    pstore::small_vector<char, 128> buffer (std::size_t{128});
    ASSERT_GE (128U, buffer.capacity ());
    std::size_t length = pstore::process_file_name (&get_process_path, buffer);
    EXPECT_EQ (length, 12U);
    buffer.resize (12U); // lose any additional buffer bytes.

    EXPECT_THAT (buffer, ::testing::ElementsAreArray (
                             {'p', 'r', 'o', 'c', 'e', 's', 's', ' ', 'p', 'a', 't', 'h'}));
}


TEST_F (ProcessFileName, GetProcessPathAlwaysReturns0) {
    std::vector<char> buffer;
    auto get_process_path = [](::pstore::gsl::span<char>) -> std::size_t { return 0; };
    check_for_error ([&]() { pstore::process_file_name (get_process_path, buffer); },
                     pstore::error_code::unknown_process_path);
}



namespace {
    // A pair of classes which together enable the sysctl() call to be mocked.
    class sysctl_base {
    public:
        virtual ~sysctl_base () {}
        virtual int sysctl (int const * name, unsigned int namelen, void * oldp,
                            std::size_t * oldlenp, void const * newp, std::size_t newlen) = 0;
    };
    class sysctl_mock2 final : public sysctl_base {
    public:
        MOCK_METHOD6 (sysctl, int(int const * name, unsigned int namelen, void * oldp,
                                  std::size_t * oldlenp, void const * newp, std::size_t newlen));
    };

    class ProcessFileNameFreeBSD : public ::testing::Test {
    public:
        ProcessFileNameFreeBSD ()
                : command_{::pstore::gsl::make_span (command_array_)} {}

    protected:
        std::vector<int> command_array_{1, 2, 3};
        ::pstore::gsl::span<int> command_;

        static auto bind (sysctl_mock2 * cb)
            -> std::function<int(int const *, unsigned int, void *, std::size_t *, void const *,
                                 std::size_t)> {
            using namespace std::placeholders;
            return std::bind (&sysctl_mock2::sysctl, cb, _1, _2, _3, _4, _5, _6);
        }
    };
} // namespace

TEST_F (ProcessFileNameFreeBSD, CommandContents) {
    sysctl_mock2 callback;

    static constexpr char const result[] = "a";
    static constexpr std::size_t const result_length = sizeof (result) / sizeof (result[0]);

    using namespace testing;

    // Handle a call where the output buffer is not large enough for the result string.
    EXPECT_CALL (callback, sysctl (_, _, _, Pointee (Lt (result_length)), _, _))
        .WillRepeatedly (DoAll (Assign (&errno, ENOMEM), Return (-1)));

    // Behaviour for calls where the output buffer is large enough.
    auto copy_string = [](int const *, unsigned int, void * oldp, std::size_t * oldlenp,
                          void const *, std::size_t) {
        std::strncpy (static_cast<char *> (oldp), result, *oldlenp);
        *oldlenp = result_length;
    };
    EXPECT_CALL (callback, sysctl (_, _, _, Pointee (Ge (result_length)), _, _))
        .WillRepeatedly (DoAll (Invoke (copy_string), Return (0)));

    std::vector<char> buffer;
    pstore::freebsd::process_file_name (command_, ProcessFileNameFreeBSD::bind (&callback), buffer);
}

TEST_F (ProcessFileNameFreeBSD, RaisesError) {
#if PSTORE_EXCEPTIONS
    sysctl_mock2 callback;

    using namespace testing;
    EXPECT_CALL (callback, sysctl (_, _, _, _, _, _))
        .Times (1)
        .WillRepeatedly (DoAll (Assign (&errno, EPERM), Return (-1)));

    using namespace std::placeholders;
    auto fn = [this, &callback]() {
        std::vector<char> buffer;
        ::pstore::freebsd::process_file_name (command_, ProcessFileNameFreeBSD::bind (&callback),
                                              buffer);
    };
    check_for_error (fn, ::pstore::errno_erc{EPERM});
#endif // PSTORE_EXCEPTIONS
}

TEST_F (ProcessFileNameFreeBSD, AlwaysRaisesNoMem) {
    sysctl_mock2 callback;

    // This test constantly raises ENONMEM indicating that the output buffer is too small. This test
    // ensures that we don't loop forever.
    using namespace testing;
    EXPECT_CALL (callback, sysctl (_, _, _, _, _, _))
        .WillRepeatedly (DoAll (Assign (&errno, ENOMEM), Return (-1)));

    std::vector<char> buffer;
    check_for_error (
        [&]() {
            pstore::freebsd::process_file_name (command_, ProcessFileNameFreeBSD::bind (&callback),
                                                buffer);
        },
        pstore::error_code::unknown_process_path);
}

TEST_F (ProcessFileNameFreeBSD, BufferContents) {
    sysctl_mock2 callback;

    static constexpr char const result[] = "process path";
    static constexpr std::size_t const result_length = sizeof (result) / sizeof (result[0]);

    // Handle a call where the output buffer is not large enough for the result string.
    using namespace testing;
    EXPECT_CALL (callback, sysctl (_, _, _, Pointee (Lt (result_length)), _, _))
        .WillRepeatedly (DoAll (Assign (&errno, ENOMEM), Return (-1)));

    // Behaviour for calls where the output buffer _is_ large enough.
    auto copy_string = [](int const *, unsigned int, void * oldp, std::size_t * oldlenp,
                          void const *, std::size_t) {
        std::strncpy (static_cast<char *> (oldp), result, *oldlenp);
        *oldlenp = std::strlen (result) + 1U;
    };
    EXPECT_CALL (callback, sysctl (_, _, _, Pointee (Ge (result_length)), _, _))
        .WillRepeatedly (DoAll (Invoke (copy_string), Return (0)));

    pstore::small_vector<char> buffer;
    std::size_t const length = pstore::freebsd::process_file_name (
        command_, ProcessFileNameFreeBSD::bind (&callback), buffer);
    EXPECT_EQ (length, 12U);
    buffer.resize (12U); // lose any additional buffer bytes.
    EXPECT_THAT (buffer, ::testing::ElementsAreArray (
                             {'p', 'r', 'o', 'c', 'e', 's', 's', ' ', 'p', 'a', 't', 'h'}));
}

TEST_F (ProcessFileNameFreeBSD, LengthIncreasesOnEachIteration) {
    std::vector<std::size_t> lengths;
    sysctl_mock2 callback;

    // Record the size of the output buffer in the 'lengths' container.
    auto record_length = [&lengths](int const *, unsigned int, void *, std::size_t * oldlenp,
                                    void const *, std::size_t) { lengths.push_back (*oldlenp); };

    static constexpr auto const output_size = std::size_t{3};

    // Returns a string containing two characters (both MAXCHAR)
    auto two_maxchars = [](int const *, unsigned int, void * oldp, std::size_t * oldlenp,
                           void const *, std::size_t) {
        *oldlenp = output_size;
        auto out = static_cast<char *> (oldp);
        out[0] = out[1] = std::numeric_limits<char>::max ();
        out[2] = '\0';
    };

    // Behaviour for calls where the output buffer is too small.
    using namespace testing;
    EXPECT_CALL (callback, sysctl (_, _, _, Pointee (Lt (output_size)), _, _))
        .WillRepeatedly (DoAll (Invoke (record_length), Assign (&errno, ENOMEM), Return (-1)));

    // Behaviour for calls where the output buffer _is_ large enough.
    EXPECT_CALL (callback, sysctl (_, _, _, Pointee (Ge (output_size)), _, _))
        .WillRepeatedly (DoAll (Invoke (record_length), Invoke (two_maxchars), Return (0)));

    std::vector<char> buffer (2);
    std::size_t const result = pstore::freebsd::process_file_name (
        command_, ProcessFileNameFreeBSD::bind (&callback), buffer);

    EXPECT_THAT (lengths.size (), Ge (2U));
    EXPECT_THAT (lengths[1], Gt (lengths[0]));
    EXPECT_THAT (lengths[1], Ge (result));

    // Finally check that the first 'result' elements of 'buffer' are MAXCHAR.
    EXPECT_THAT (buffer.size (), Ge (result));
    buffer.resize (result);

    auto const c = std::numeric_limits<char>::max ();
    EXPECT_THAT (buffer, ElementsAre (c, c));
}
