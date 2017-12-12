//*                                       *
//*  _ __ ___   ___ _ __ ___ _ __   ___   *
//* | '_ ` _ \ / __| '__/ _ \ '_ \ / _ \  *
//* | | | | | | (__| | |  __/ |_) | (_) | *
//* |_| |_| |_|\___|_|  \___| .__/ \___/  *
//*                         |_|           *
//===- unittests/dump/test_mcrepo.cpp -------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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

#include "dump/mcrepo_value.hpp"
#include "pstore/db_archive.hpp"
#include "pstore/serialize/standard_types.hpp"
#include "pstore/transaction.hpp"

#include "split.hpp"

#include <memory>
#include <vector>

#include "gmock/gmock.h"

using namespace pstore::repo;


#ifdef _WIN32

std::shared_ptr<std::uint8_t> aligned_valloc (std::size_t size, unsigned align) {
    size += align - 1U;

    auto ptr = reinterpret_cast<std::uint8_t *> (
        ::VirtualAlloc (nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (ptr == nullptr) {
        DWORD const last_error = ::GetLastError ();
        raise (pstore::win32_erc (last_error), "VirtualAlloc");
    }

    auto deleter = [ptr](std::uint8_t *) { ::VirtualFree (ptr, 0, MEM_RELEASE); };
    auto const mask = ~(std::uintptr_t{align} - 1);
    auto ptr_aligned = reinterpret_cast<std::uint8_t *> (
        (reinterpret_cast<std::uintptr_t> (ptr) + align - 1) & mask);
    return std::shared_ptr<std::uint8_t> (ptr_aligned, deleter);
}

#else

#include <sys/mman.h>

// MAP_ANONYMOUS isn't defined by POSIX, but both macOS and Linux support it.
// Earlier versions of macOS provided the MAP_ANON name for the flag.
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

std::shared_ptr<std::uint8_t> aligned_valloc (std::size_t size, unsigned align) {
    size += align - 1U;

    auto ptr = ::mmap (nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == nullptr) {
        raise (pstore::errno_erc{errno}, "mmap");
    }

    auto deleter = [ptr, size](std::uint8_t *) { ::munmap (ptr, size); };
    auto const mask = ~(std::uintptr_t{align} - 1);
    auto ptr_aligned = reinterpret_cast<std::uint8_t *> (
        (reinterpret_cast<std::uintptr_t> (ptr) + align - 1) & mask);
    return std::shared_ptr<std::uint8_t> (ptr_aligned, deleter);
}

#endif // _WIN32


// *******************************************
// *                                         *
// *              MCRepoFixture              *
// *                                         *
// *******************************************

namespace {
    class MCRepoFixture : public ::testing::Test {
        struct mock_mutex {
        public:
            void lock () {}
            void unlock () {}
        };

    public:
        void SetUp () override;
        void TearDown () override;

        using lock_guard = std::unique_lock<mock_mutex>;
        using transaction_type = pstore::transaction<lock_guard>;

    protected:
        pstore::address store_str (transaction_type & transaction, std::string const & str);

        mock_mutex mutex_;
        std::size_t const file_size_ = pstore::storage::min_region_size * 2;
        std::shared_ptr<std::uint8_t> buffer_;
        std::shared_ptr<pstore::file::in_memory> file_;
        std::unique_ptr<pstore::database> db_;
    };

    // SetUp
    // ~~~~~
    pstore::address MCRepoFixture::store_str (transaction_type & transaction,
                                              std::string const & str) {
        auto archive = pstore::serialize::archive::make_writer (transaction);
        return pstore::serialize::write (archive, str);
    }

    // SetUp
    // ~~~~~
    void MCRepoFixture::SetUp () {
        // Build an empty, in-memory database.
        constexpr std::size_t page_size = 4096;
        buffer_ = aligned_valloc (file_size_, page_size);
        file_ = std::make_shared<pstore::file::in_memory> (buffer_, file_size_);
        pstore::database::build_new_store (*file_);
        db_.reset (new pstore::database (file_));
        db_->set_vacuum_mode (pstore::database::vacuum_mode::disabled);
    }

    // TearDown
    // ~~~~~~~~
    void MCRepoFixture::TearDown () {
        buffer_.reset ();
        file_.reset ();
        db_.reset ();
    }
} // namespace


TEST_F (MCRepoFixture, DumpFragment) {
    using ::testing::ElementsAre;

    transaction_type transaction = pstore::begin (*db_, lock_guard{mutex_});

    std::array<section_content, 1> c = {
        {section_content (section_type::Text, std::uint8_t{4} /*alignment*/)}};
    {
        // Build the text section's contents and fixups.
        section_content & text = c.back ();
        text.data.assign ({'t', 'e', 'x', 't'});
        text.ifixups.emplace_back (internal_fixup{section_type::Data, 2, 2, 2});
        text.xfixups.emplace_back (external_fixup{this->store_str (transaction, "foo"), 3, 3, 3});
    }

    auto fragment =
        fragment::load (*db_, fragment::alloc (transaction, std::begin (c), std::end (c)));

    std::ostringstream out;
    value::value_ptr addr = value::make_value (*db_, *fragment);
    addr->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (16U, lines.size ());

    auto line = 0U;
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ());
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("-", "type", ":", "Text"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("contents", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("align", ":", "0x10"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("data", ":", "!!binary", "|"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("dGV4dA=="));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("ifixups", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("-", "section", ":", "Data"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("type", ":", "0x2"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("offset", ":", "0x2"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("addend", ":", "0x2"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("xfixups", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("-", "name", ":", "foo"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("type", ":", "0x3"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("offset", ":", "0x3"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("addend", ":", "0x3"));
}

TEST_F (MCRepoFixture, DumpTicket) {
    using ::testing::ElementsAre;

    // Write output file path "/home/user/test.c"
    transaction_type transaction = pstore::begin (*db_, lock_guard{mutex_});

    // ticket_member sm{digest, name, linkage};
    std::vector<ticket_member> v{{pstore::index::digest{28U}, this->store_str (transaction, "main"),
                                  linkage_type::external}};
    auto ticket = ticket::load (
        *db_, ticket::alloc (transaction, this->store_str (transaction, "/home/user/"), v));

    std::ostringstream out;
    value::value_ptr addr = value::make_value (*db_, ticket);
    addr->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (6U, lines.size ());

    auto line = 0U;
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("members", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("ticket_member", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("-", "digest", ":", "0000000000000000000000000000001c"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("name", ":", "main"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("linkage", ":", "external"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("path", ":", "/home/user/"));
}

// eof: unittests/dump/test_mcrepo.cpp
