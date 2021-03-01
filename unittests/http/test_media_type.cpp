//===- unittests/http/test_media_type.cpp ---------------------------------===//
//*                     _ _         _                     *
//*  _ __ ___   ___  __| (_) __ _  | |_ _   _ _ __   ___  *
//* | '_ ` _ \ / _ \/ _` | |/ _` | | __| | | | '_ \ / _ \ *
//* | | | | | |  __/ (_| | | (_| | | |_| |_| | |_) |  __/ *
//* |_| |_| |_|\___|\__,_|_|\__,_|  \__|\__, | .__/ \___| *
//*                                     |___/|_|          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/http/media_type.hpp"
#include <gtest/gtest.h>

TEST (MediaType, Empty) {
    EXPECT_STREQ (pstore::http::media_type_from_extension (""), "application/octet-stream");
    EXPECT_STREQ (pstore::http::media_type_from_filename (""), "application/octet-stream");
}

TEST (MediaType, JSON) {
    EXPECT_STREQ (pstore::http::media_type_from_extension (".json"), "application/json");
    EXPECT_STREQ (pstore::http::media_type_from_filename ("foo.json"), "application/json");
}

TEST (MediaType, Unknown) {
    EXPECT_STREQ (pstore::http::media_type_from_extension (".foo"), "application/octet-stream");
    EXPECT_STREQ (pstore::http::media_type_from_filename ("foo.foo"), "application/octet-stream");
}
