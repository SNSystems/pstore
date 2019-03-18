//*                                _          _                  *
//*   __ _ _   _  ___ _ __ _   _  | |_ ___   | | ____   ___ __   *
//*  / _` | | | |/ _ \ '__| | | | | __/ _ \  | |/ /\ \ / / '_ \  *
//* | (_| | |_| |  __/ |  | |_| | | || (_) | |   <  \ V /| |_) | *
//*  \__, |\__,_|\___|_|   \__, |  \__\___/  |_|\_\  \_/ | .__/  *
//*     |_|                |___/                         |_|     *
//===- unittests/httpd/test_query_to_kvp.cpp ------------------------------===//
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
#include "pstore/httpd/query_to_kvp.hpp"

#include <map>
#include <string>

#include <gmock/gmock.h>

using ::testing::ContainerEq;
using string_map = std::map<std::string, std::string>;

TEST (QueryToKvp, EmptyString) {
    string_map result;
    query_to_kvp ("", pstore::httpd::make_insert_iterator (result));
    EXPECT_THAT (result, ContainerEq (string_map{}));
}

TEST (QueryToKvp, SingleKvp) {
    string_map result;
    query_to_kvp ("key=value", pstore::httpd::make_insert_iterator (result));
    EXPECT_THAT (result, ContainerEq (string_map{{"key", "value"}}));
}

TEST (QueryToKvp, TwoKvps) {
    string_map result;
    query_to_kvp ("a=1&b=2", pstore::httpd::make_insert_iterator (result));
    EXPECT_THAT (result, ContainerEq (string_map{{"a", "1"}, {"b", "2"}}));
}

TEST (QueryToKvp, TwoKvpsSemicolonSeparator) {
    string_map result;
    query_to_kvp ("a=1;b=2", pstore::httpd::make_insert_iterator (result));
    EXPECT_THAT (result, ContainerEq (string_map{{"a", "1"}, {"b", "2"}}));
}

TEST (QueryToKvp, BadQueryStringValue) {
    string_map result;
    query_to_kvp ("param1=hello=world&param2=false", pstore::httpd::make_insert_iterator (result));
    EXPECT_THAT (result, ContainerEq (string_map{{"param1", "hello=world"}, {"param2", "false"}}));
}

TEST (QueryToKvp, MissingValue) {
    string_map result;
    query_to_kvp ("param1=&param2=false", pstore::httpd::make_insert_iterator (result));
    EXPECT_THAT (result, ContainerEq (string_map{{"param1", ""}, {"param2", "false"}}));
}

TEST (QueryToKvp, DuplicateKeyIgnored) {
    string_map result;
    query_to_kvp ("k1=v1&k1=v2", pstore::httpd::make_insert_iterator (result));
    EXPECT_THAT (result, ContainerEq (string_map{{"k1", "v1"}}));
}

TEST (QueryToKvp, HashTerminatesQuery) {
    string_map result;
    std::string const str{"k1=v1&k2=v2#foo"};
    std::string::const_iterator pos =
        query_to_kvp (str, pstore::httpd::make_insert_iterator (result));
    EXPECT_EQ (std::string (pos, std::end (str)), "foo");
    EXPECT_THAT (result, ContainerEq (string_map{{"k1", "v1"}, {"k2", "v2"}}));
}



TEST (KvpToQuery, Empty) {
    string_map const in;
    std::string out;
    pstore::httpd::kvp_to_query (std::begin (in), std::end (in), std::back_inserter (out));
    EXPECT_EQ (out, "");
}

TEST (KvpToQuery, SingleKvp) {
    string_map const in{{"key", "value"}};
    std::string out;
    pstore::httpd::kvp_to_query (std::begin (in), std::end (in), std::back_inserter (out));
    EXPECT_EQ (out, "key=value");
}

TEST (KvpToQuery, TwoKvp2) {
    string_map const in{{"k1", "v1"}, {"k2", "v2"}};
    std::string out;
    pstore::httpd::kvp_to_query (std::begin (in), std::end (in), std::back_inserter (out));
    EXPECT_EQ (out, "k1=v1&k2=v2");
}

TEST (KvpToQuery, KvpNeedingEscape) {
    string_map const in{{"_", "&"}, {"/", "v2"}};
    std::string out;
    pstore::httpd::kvp_to_query (std::begin (in), std::end (in), std::back_inserter (out));
    EXPECT_EQ (out, "%2F=v2&_=%26");
}
