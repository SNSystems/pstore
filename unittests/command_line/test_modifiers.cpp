//*                      _ _  __ _                *
//*  _ __ ___   ___   __| (_)/ _(_) ___ _ __ ___  *
//* | '_ ` _ \ / _ \ / _` | | |_| |/ _ \ '__/ __| *
//* | | | | | | (_) | (_| | |  _| |  __/ |  \__ \ *
//* |_| |_| |_|\___/ \__,_|_|_| |_|\___|_|  |___/ *
//*                                               *
//===- unittests/command_line/test_modifiers.cpp --------------------------===//
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
#include "pstore/command_line/modifiers.hpp"

#include <sstream>
#include <string>
#include <vector>

#include <gmock/gmock.h>

#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/option.hpp"

using namespace pstore::command_line;
using testing::HasSubstr;
using testing::Not;

namespace {

    enum class enumeration : int { a, b, c };

#if defined(_WIN32) && defined(_UNICODE)
    using string_stream = std::wostringstream;
#else
    using string_stream = std::ostringstream;
#endif

    class Modifiers : public testing::Test {
    public:
        ~Modifiers () { option::reset_container (); }
    };

} // end anonymous namespace

TEST_F (Modifiers, DefaultConstruction) {
    opt<enumeration> opt;
    EXPECT_EQ (opt.get (), enumeration::a);
}

TEST_F (Modifiers, Init) {
    // init() allows the initial (default) value of the option to be described.
    opt<enumeration> opt_a (init (enumeration::a));

    EXPECT_EQ (opt_a.get (), enumeration::a);

    opt<enumeration> opt_b (init (enumeration::b));
    EXPECT_EQ (opt_b.get (), enumeration::b);
}


namespace {

    class EnumerationParse : public testing::Test {
    public:
        EnumerationParse ()
                : enum_opt_{
                      "enumeration",
                      values (literal{"a", static_cast<int> (enumeration::a), "a description"},
                              literal{"b", static_cast<int> (enumeration::b), "b description"},
                              literal{"c", static_cast<int> (enumeration::c), "c description"})} {}
        ~EnumerationParse () { option::reset_container (); }

    protected:
        opt<enumeration> enum_opt_;
    };

} // end anonymous namespace

TEST_F (EnumerationParse, SetA) {
    std::vector<std::string> argv{"progname", "--enumeration=a"};
    string_stream output;
    string_stream errors;
    bool ok = details::parse_command_line_options (std::begin (argv), std::end (argv), "overview",
                                                   output, errors);
    ASSERT_TRUE (ok);
    ASSERT_EQ (enum_opt_.get (), enumeration::a);
}

TEST_F (EnumerationParse, SetC) {
    std::vector<std::string> argv{"progname", "--enumeration=c"};
    string_stream output;
    string_stream errors;
    bool ok = details::parse_command_line_options (std::begin (argv), std::end (argv), "overview",
                                                   output, errors);
    ASSERT_TRUE (ok);
    ASSERT_EQ (enum_opt_.get (), enumeration::c);
}

TEST_F (EnumerationParse, ErrorBadValue) {
    std::vector<std::string> argv{"progname", "--enumeration=bad"};
    string_stream output;
    string_stream errors;
    bool ok = details::parse_command_line_options (std::begin (argv), std::end (argv), "overview",
                                                   output, errors);
    ASSERT_FALSE (ok);
    EXPECT_THAT (errors.str (), HasSubstr (NATIVE_TEXT ("'bad'")));
}

TEST_F (EnumerationParse, GoodValueAfterError) {
    std::vector<std::string> argv{"progname", "--unknown", "--enumeration=a"};
    string_stream output;
    string_stream errors;
    bool ok = details::parse_command_line_options (std::begin (argv), std::end (argv), "overview",
                                                   output, errors);
    ASSERT_FALSE (ok);
    EXPECT_THAT (errors.str (), Not (HasSubstr (NATIVE_TEXT ("'a'"))));
}
