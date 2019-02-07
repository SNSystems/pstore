//*  _            __  __                   _                      _            *
//* | |__  _   _ / _|/ _| ___ _ __ ___  __| |  _ __ ___  __ _  __| | ___ _ __  *
//* | '_ \| | | | |_| |_ / _ \ '__/ _ \/ _` | | '__/ _ \/ _` |/ _` |/ _ \ '__| *
//* | |_) | |_| |  _|  _|  __/ | |  __/ (_| | | | |  __/ (_| | (_| |  __/ |    *
//* |_.__/ \__,_|_| |_|  \___|_|  \___|\__,_| |_|  \___|\__,_|\__,_|\___|_|    *
//*                                                                            *
//===- unittests/httpd/test_buffered_reader.cpp ---------------------------===//
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
#include "pstore/httpd/buffered_reader.hpp"

#include <cassert>
#include <queue>

#include "gmock/gmock.h"

#include "pstore/support/array_elements.hpp"

namespace {

    class mock_refiller {
    public:
        virtual ~mock_refiller () = default;
        virtual pstore::error_or<char *> fill (char * first, char * last) = 0;
    };

    class refiller : public mock_refiller {
    public:
        MOCK_METHOD2 (fill, pstore::error_or<char *> (char *, char *));
        std::queue<std::function<pstore::error_or<char *> (char *, char *)>> fns;
    };
} // namespace

TEST (HttpdBufferedReader, GetcThenEOF) {
    using pstore::error_or;
    using pstore::maybe;
    using testing::_;
    using testing::Invoke;

    refiller r;
    int call_count = 0;
    EXPECT_CALL (r, fill (_, _))
        .Times (2)
        .WillRepeatedly (Invoke ([&call_count](char * first, char * last) {
            switch (++call_count) {
            case 1:
                *(first++) = 'a';
                assert (first < last);
                break;
            default: first = nullptr; break;
            }
            return error_or<char *> (pstore::in_place, first);
        }));

    auto br = pstore::httpd::make_buffered_reader (
        [&r](char * first, char * last) { return r.fill (first, last); });

    {
        error_or<maybe<char>> const c1 = br.getc ();
        ASSERT_FALSE (c1.has_error ());
        ASSERT_TRUE (c1.get_value ().has_value ());
        EXPECT_EQ (c1.get_value ().value (), 'a');
    }
    {
        error_or<maybe<char>> const c2 = br.getc ();
        ASSERT_FALSE (c2.has_error ());
        ASSERT_FALSE (c2.get_value ().has_value ());
    }
}

TEST (HttpdBufferedReader, GetTwoStringsLFThenEOF) {
    using pstore::error_or;
    using pstore::maybe;
    using testing::_;
    using testing::Invoke;

    refiller r;
    r.fns.push ([](char * first, char * last) {
        static char str[] = "abc\ndef";
        first = std::copy_n (str, pstore::array_elements (str) - 1U, first);
        assert (first < last);
        return error_or<char *> (pstore::in_place, first);
    });
    r.fns.push ([](char *, char *) { return error_or<char *> (pstore::in_place, nullptr); });

    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke ([&r](char * first, char * last) {
        assert (!r.fns.empty ());
        auto f = r.fns.front ();
        r.fns.pop ();
        return f (first, last);
    }));

    auto br = pstore::httpd::make_buffered_reader (
        [&r](char * first, char * last) { return r.fill (first, last); });

    {
        error_or<maybe<std::string>> const s1 = br.gets ();
        ASSERT_FALSE (s1.has_error ());
        ASSERT_TRUE (s1.get_value ().has_value ());
        EXPECT_EQ (s1.get_value ().value (), "abc");
    }
    {
        error_or<maybe<std::string>> const s2 = br.gets ();
        ASSERT_FALSE (s2.has_error ());
        ASSERT_TRUE (s2.get_value ().has_value ());
        EXPECT_EQ (s2.get_value ().value (), "def");
    }
    {
        error_or<maybe<std::string>> const s3 = br.gets ();
        ASSERT_FALSE (s3.has_error ());
        ASSERT_FALSE (s3.get_value ().has_value ());
    }
}

TEST (HttpdBufferedReader, StringCRLF) {
    using pstore::error_or;
    using pstore::maybe;
    using testing::_;
    using testing::Invoke;

    refiller r;
    r.fns.push ([](char * first, char * last) {
        static char str[] = "abc\r\n";
        first = std::copy_n (str, pstore::array_elements (str) - 1U, first);
        assert (first < last);
        return error_or<char *> (pstore::in_place, first);
    });
    r.fns.push ([](char *, char *) { return error_or<char *> (pstore::in_place, nullptr); });

    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke ([&r](char * first, char * last) {
        assert (!r.fns.empty ());
        auto f = r.fns.front ();
        r.fns.pop ();
        return f (first, last);
    }));

    auto br = pstore::httpd::make_buffered_reader (
        [&r](char * first, char * last) { return r.fill (first, last); });
    {
        error_or<maybe<std::string>> const s1 = br.gets ();
        ASSERT_FALSE (s1.has_error ());
        ASSERT_TRUE (s1.get_value ().has_value ());
        EXPECT_EQ (s1.get_value ().value (), "abc");
    }
    {
        error_or<maybe<std::string>> const s3 = br.gets ();
        ASSERT_FALSE (s3.has_error ());
        ASSERT_FALSE (s3.get_value ().has_value ());
    }
}

TEST (HttpdBufferedReader, StringCRNoLF) {
    using pstore::error_or;
    using pstore::maybe;
    using testing::_;
    using testing::Invoke;

    refiller r;
    std::queue<std::function<error_or<char *> (char *, char *)>> fns;
    fns.push ([](char * first, char * last) {
        static char str[] = "abc\r";
        first = std::copy_n (str, pstore::array_elements (str) - 1U, first);
        assert (first < last);
        return error_or<char *> (pstore::in_place, first);
    });
    auto eof_func = [](char *, char *) { return error_or<char *> (pstore::in_place, nullptr); };
    fns.push (eof_func);
    fns.push (eof_func);

    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke ([&fns](char * first, char * last) {
        assert (!fns.empty ());
        auto f = fns.front ();
        fns.pop ();
        return f (first, last);
    }));

    auto br = pstore::httpd::make_buffered_reader (
        [&r](char * first, char * last) { return r.fill (first, last); });
    {
        error_or<maybe<std::string>> const s1 = br.gets ();
        ASSERT_FALSE (s1.has_error ());
        ASSERT_TRUE (s1.get_value ().has_value ());
        EXPECT_EQ (s1.get_value ().value (), "abc");
    }
    {
        error_or<maybe<std::string>> const s3 = br.gets ();
        ASSERT_FALSE (s3.has_error ());
        ASSERT_FALSE (s3.get_value ().has_value ());
    }
}
