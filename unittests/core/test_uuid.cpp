//*              _     _  *
//*  _   _ _   _(_) __| | *
//* | | | | | | | |/ _` | *
//* | |_| | |_| | | (_| | *
//*  \__,_|\__,_|_|\__,_| *
//*                       *
//===- unittests/core/test_uuid.cpp ---------------------------------------===//
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

#include "pstore/core/uuid.hpp"
#include "pstore/support/portab.hpp"
#include <gmock/gmock.h>
#include "check_for_error.hpp"

namespace {

    class BasicUUID : public ::testing::Test {
    protected:
        pstore::uuid id_{
            pstore::uuid::container_type{{0x84, 0x94, 0x9c, 0xc5, 0x47, 0x01, 0x4a, 0x84, 0x89,
                                          0x5b, 0x35, 0x4c, 0x58, 0x4a, 0x98, 0x1b}}};
    };

} // end anonymous namespace

TEST_F (BasicUUID, Parse) {
    pstore::uuid const t1 ("84949cc5-4701-4a84-895b-354c584a981b");
    EXPECT_EQ (t1, id_);
    EXPECT_FALSE (t1.is_null ());
}

TEST_F (BasicUUID, Version) {
    EXPECT_EQ (pstore::uuid::version_type::random_number_based, id_.version ());
}

TEST_F (BasicUUID, Variant) {
    EXPECT_EQ (pstore::uuid::variant_type::rfc_4122, id_.variant ());
}

TEST_F (BasicUUID, String) {
    EXPECT_EQ ("84949cc5-4701-4a84-895b-354c584a981b", id_.str ());
}

TEST_F (BasicUUID, Out) {
    std::ostringstream str;
    str << id_;
    EXPECT_EQ ("84949cc5-4701-4a84-895b-354c584a981b", str.str ());
}

TEST (UUID, ParseStringBadLength) {
    {
        std::string const input1 = "00000000-0000-0000-0000-00000000000000";
        pstore::maybe<pstore::uuid> v1 = pstore::uuid::from_string (input1);
        EXPECT_FALSE (v1.has_value ());

        check_for_error ([&input1] () { pstore::uuid _ (input1); },
                         pstore::error_code::uuid_parse_error);
    }
    {
        std::string const input2 = "00000000-0000-0000-0000-0000000000";
        pstore::maybe<pstore::uuid> v2 = pstore::uuid::from_string (input2);
        EXPECT_FALSE (v2.has_value ());

        check_for_error ([&input2] () { pstore::uuid _ (input2); },
                         pstore::error_code::uuid_parse_error);
    }
}

TEST (UUID, MissingDash) {
    EXPECT_FALSE (pstore::uuid::from_string ("0000000000000-0000-0000-000000000000").has_value ());
    EXPECT_FALSE (pstore::uuid::from_string ("00000000-000000000-0000-000000000000").has_value ());
    EXPECT_FALSE (pstore::uuid::from_string ("00000000-0000-000000000-000000000000").has_value ());
    EXPECT_FALSE (pstore::uuid::from_string ("00000000-0000-0000-00000000000000000").has_value ());
}

TEST (UUID, GoodHex) {
    std::string const input = "0099aaff-AAFF-0990-0099-aaffAAFF0990";
    pstore::uuid const expected{
        pstore::uuid::container_type{{0x00, 0x99, 0xAA, 0xFF, 0xAA, 0xFF, 0x09, 0x90, 0x00, 0x99,
                                      0xAA, 0xFF, 0xAA, 0xFF, 0x09, 0x90}}};

    // first try from_string()...
    pstore::maybe<pstore::uuid> v = pstore::uuid::from_string (input);
    ASSERT_TRUE (v.has_value ());
    EXPECT_EQ (*v, expected);

    // now using the constructor
    EXPECT_EQ (expected, pstore::uuid (input));
}

TEST (UUID, BadHex) {
    // '/' is one behind '0' in ASCII
    EXPECT_FALSE (pstore::uuid::from_string ("/0000000-0000-0000-0000-000000000000").has_value ());
    // ':' is one past '9' in ASCII
    EXPECT_FALSE (pstore::uuid::from_string (":0000000-0000-0000-0000-000000000000").has_value ());
    // '@' is one behind 'A' in ASCII
    EXPECT_FALSE (pstore::uuid::from_string ("@0000000-0000-0000-0000-000000000000").has_value ());
    // 'G' is one past 'F'. Too obvious?
    EXPECT_FALSE (pstore::uuid::from_string ("G0000000-0000-0000-0000-000000000000").has_value ());
    EXPECT_FALSE (pstore::uuid::from_string ("0G000000-0000-0000-0000-000000000000").has_value ());
    EXPECT_FALSE (pstore::uuid::from_string ("g0000000-0000-0000-0000-000000000000").has_value ());
    EXPECT_FALSE (pstore::uuid::from_string ("0g000000-0000-0000-0000-000000000000").has_value ());
    // '`' is one behind 'a' in ASCII
    EXPECT_FALSE (pstore::uuid::from_string ("`0000000-0000-0000-0000-000000000000").has_value ());
}

TEST (UUID, MixedCase) {
    EXPECT_EQ (pstore::uuid ("aaaaaaaa-AAAA-aAAa-FFff-fFFf00000000"),
               pstore::uuid (
                   pstore::uuid::container_type{{0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
                                                 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00}}));
}

TEST (UUID, Null) {
    pstore::uuid const t1 ("00000000-0000-0000-0000-000000000000");
    EXPECT_TRUE (t1.is_null ());
}

TEST (UUID, Create) {
    pstore::uuid const t;
    EXPECT_FALSE (t.is_null ());
    EXPECT_EQ (pstore::uuid::version_type::random_number_based, t.version ());
    EXPECT_EQ (pstore::uuid::variant_type::rfc_4122, t.variant ());
}


TEST (UUID, VersionTimeBased) {
    pstore::uuid const t1 ("FFFFFFFF-FFFF-1FFF-FFFF-FFFFFFFFFFFF");
    EXPECT_EQ (pstore::uuid::version_type::time_based, t1.version ());
}
TEST (UUID, VersionDCESecurity) {
    pstore::uuid const t1 ("FFFFFFFF-FFFF-2FFF-FFFF-FFFFFFFFFFFF");
    EXPECT_EQ (pstore::uuid::version_type::dce_security, t1.version ());
}
TEST (UUID, VersionNameBasedMD5) {
    pstore::uuid const t1 ("ffffffff-ffff-3fff-ffff-ffffffffffff");
    EXPECT_EQ (pstore::uuid::version_type::name_based_md5, t1.version ());
}
TEST (UUID, VersionRandomNumberBased) {
    pstore::uuid const t1 ("ffffffff-ffff-4fff-ffff-ffffffffffff");
    EXPECT_EQ (pstore::uuid::version_type::random_number_based, t1.version ());
}
TEST (UUID, VersionNameBasedSHA1) {
    pstore::uuid const t1 ("ffffffff-ffff-5fff-ffff-ffffffffffff");
    EXPECT_EQ (pstore::uuid::version_type::name_based_sha1, t1.version ());
}
TEST (UUID, VersionUnknown) {
    pstore::uuid const t1 ("ffffffff-ffff-ffff-ffff-ffffffffffff");
    EXPECT_EQ (pstore::uuid::version_type::unknown, t1.version ());
}


TEST (UUID, Variant) {
    pstore::uuid const t1 ("ffffffff-ffff-ffff-7fff-ffffffffffff");
    EXPECT_EQ (pstore::uuid::variant_type::ncs, t1.variant ());
}
TEST (UUID, VariantRFC4122) {
    pstore::uuid const t1 ("ffffffff-ffff-ffff-bfff-ffffffffffff");
    EXPECT_EQ (pstore::uuid::variant_type::rfc_4122, t1.variant ());
}
TEST (UUID, VariantMicrosoft) {
    pstore::uuid const t1 ("ffffffff-ffff-ffff-cfff-ffffffffffff");
    EXPECT_EQ (pstore::uuid::variant_type::microsoft, t1.variant ());
}
TEST (UUID, VariantFuture) {
    pstore::uuid const t1 ("ffffffff-ffff-ffff-ffff-ffffffffffff");
    EXPECT_EQ (pstore::uuid::variant_type::future, t1.variant ());
}


#ifdef _WIN32

TEST (UUID, FromNativeType) {
    UUID native;
    native.Data1 = 0x00112233UL;
    native.Data2 = 0x4455;
    native.Data3 = 0x6677;
    native.Data4[0] = 0x88;
    native.Data4[1] = 0x99;
    native.Data4[2] = 0xaa;
    native.Data4[3] = 0xbb;
    native.Data4[4] = 0xcc;
    native.Data4[5] = 0xdd;
    native.Data4[6] = 0xee;
    native.Data4[7] = 0xff;

    pstore::uuid const expected{"00112233-4455-6677-8899-aabbccddeeff"};
    pstore::uuid actual{native};
    EXPECT_EQ (expected, actual);
}

#elif defined(__APPLE__)

TEST (UUID, FromNativeType) {
    uuid_t native{0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                  0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    pstore::uuid const expected{"00112233-4455-6677-8899-aabbccddeeff"};
    pstore::uuid actual{native};
    EXPECT_EQ (expected, actual);
}

#endif


namespace {

    class CompareUUID : public ::testing::Test {
    protected:
        pstore::uuid t1_{"00000000-0000-4a00-8900-000000000000"};
        pstore::uuid t2_{"00000000-0000-4a00-8900-000000000000"};
        pstore::uuid t3_{"00000000-0000-4a00-8900-000000000001"};
    };
} // namespace

TEST_F (CompareUUID, Eq) {
    EXPECT_TRUE (t1_ == t2_);
    EXPECT_FALSE (t1_ == t3_);
}
TEST_F (CompareUUID, Ne) {
    EXPECT_FALSE (t1_ != t2_);
    EXPECT_TRUE (t1_ != t3_);
}
TEST_F (CompareUUID, Lt) {
    EXPECT_TRUE (t1_ < t3_);
    EXPECT_FALSE (t1_ < t2_);
}
TEST_F (CompareUUID, Le) {
    EXPECT_TRUE (t1_ <= t3_);
    EXPECT_TRUE (t1_ <= t2_);
    EXPECT_FALSE (t3_ <= t1_);
}
TEST_F (CompareUUID, Gt) {
    EXPECT_TRUE (t3_ > t1_);
    EXPECT_FALSE (t2_ > t1_);
}
TEST_F (CompareUUID, Ge) {
    EXPECT_TRUE (t3_ >= t1_);
    EXPECT_TRUE (t2_ >= t1_);
    EXPECT_FALSE (t1_ >= t3_);
}
