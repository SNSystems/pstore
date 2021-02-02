//*               _               *
//* __      _____| | _____ _   _  *
//* \ \ /\ / / __| |/ / _ \ | | | *
//*  \ V  V /\__ \   <  __/ |_| | *
//*   \_/\_/ |___/_|\_\___|\__, | *
//*                        |___/  *
//===- unittests/http/test_wskey.cpp --------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/http/wskey.hpp"

#include <gmock/gmock.h>

#include "pstore/support/array_elements.hpp"
#include "pstore/support/portab.hpp"

TEST (Sha1, Test1) {
    char const test[] = {'a', 'b', 'c'};
    PSTORE_STATIC_ASSERT (sizeof (char) == sizeof (std::uint8_t));

    pstore::http::sha1 sha;
    sha.input (pstore::gsl::make_span (reinterpret_cast<std::uint8_t const *> (test),
                                       pstore::array_elements (test)));
    pstore::http::sha1::result_type const digest = sha.result ();
    EXPECT_THAT (digest, testing::ContainerEq (pstore::http::sha1::result_type{
                             {0xA9, 0x99, 0x3E, 0x36, 0x47, 0x06, 0x81, 0x6A, 0xBA, 0x3E,
                              0x25, 0x71, 0x78, 0x50, 0xC2, 0x6C, 0x9C, 0xD0, 0xD8, 0x9D}}));

    EXPECT_EQ (pstore::http::sha1::digest_to_base64 (digest), "qZk+NkcGgWq6PiVxeFDCbJzQ2J0=");
}

TEST (Sha1, Test2) {
    char const testa[] = {'a', 'b', 'c', 'd', 'b', 'c', 'd', 'e', 'c', 'd',
                          'e', 'f', 'd', 'e', 'f', 'g', 'e', 'f', 'g', 'h',
                          'f', 'g', 'h', 'i', 'g', 'h', 'i', 'j', 'h', 'i'};
    char const testb[] = {'j', 'k', 'i', 'j', 'k', 'l', 'j', 'k', 'l', 'm', 'k', 'l', 'm',
                          'n', 'l', 'm', 'n', 'o', 'm', 'n', 'o', 'p', 'n', 'o', 'p', 'q'};

    pstore::http::sha1 sha;
    sha.input (pstore::gsl::make_span (reinterpret_cast<std::uint8_t const *> (testa),
                                       pstore::array_elements (testa)));
    sha.input (pstore::gsl::make_span (reinterpret_cast<std::uint8_t const *> (testb),
                                       pstore::array_elements (testb)));
    pstore::http::sha1::result_type const digest = sha.result ();
    EXPECT_THAT (digest, testing::ContainerEq (pstore::http::sha1::result_type{
                             {0x84, 0x98, 0x3E, 0x44, 0x1C, 0x3B, 0xD2, 0x6E, 0xBA, 0xAE,
                              0x4A, 0xA1, 0xF9, 0x51, 0x29, 0xE5, 0xE5, 0x46, 0x70, 0xF1}}));
    EXPECT_EQ (pstore::http::sha1::digest_to_base64 (digest), "hJg+RBw70m66rkqh+VEp5eVGcPE=");
}

TEST (Sha1, Test3) {
    char const test[] = {'a'};
    auto const span = pstore::gsl::make_span (reinterpret_cast<std::uint8_t const *> (test),
                                              pstore::array_elements (test));
    pstore::http::sha1 sha;
    for (auto ctr = 0U; ctr < 1000000U; ++ctr) {
        sha.input (span);
    }
    pstore::http::sha1::result_type const digest = sha.result ();
    EXPECT_THAT (digest, testing::ContainerEq (pstore::http::sha1::result_type{
                             {0x34, 0xAA, 0x97, 0x3C, 0xD4, 0xC4, 0xDA, 0xA4, 0xF6, 0x1E,
                              0xEB, 0x2B, 0xDB, 0xAD, 0x27, 0x31, 0x65, 0x34, 0x01, 0x6F}}));
    EXPECT_EQ (pstore::http::sha1::digest_to_base64 (digest), "NKqXPNTE2qT2Husr260nMWU0AW8=");
}

TEST (Sha1, Test4) {
    char const test[] = {'0', '1', '2', '3', '4', '5', '6', '7', '0', '1', '2',
                         '3', '4', '5', '6', '7', '0', '1', '2', '3', '4', '5',
                         '6', '7', '0', '1', '2', '3', '4', '5', '6', '7'};

    pstore::http::sha1 sha;
    for (auto ctr = 0U; ctr < 10U; ++ctr) {
        auto const span = pstore::gsl::make_span (reinterpret_cast<std::uint8_t const *> (test),
                                                  pstore::array_elements (test));
        sha.input (span);
        sha.input (span);
    }
    pstore::http::sha1::result_type const digest = sha.result ();
    EXPECT_THAT (digest, testing::ContainerEq (pstore::http::sha1::result_type{
                             {0xDE, 0xA3, 0x56, 0xA2, 0xCD, 0xDD, 0x90, 0xC7, 0xA7, 0xEC,
                              0xED, 0xC5, 0xEB, 0xB5, 0x63, 0x93, 0x4F, 0x46, 0x04, 0x52}}));
    EXPECT_EQ (pstore::http::sha1::digest_to_base64 (digest), "3qNWos3dkMen7O3F67Vjk09GBFI=");
}

TEST (Sha1, Handshake) {
    EXPECT_EQ (pstore::http::source_key ("dGhlIHNhbXBsZSBub25jZQ=="),
               "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}
