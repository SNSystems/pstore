//===- unittests/romfs/test_romfs.cpp -------------------------------------===//
//*                       __      *
//*  _ __ ___  _ __ ___  / _|___  *
//* | '__/ _ \| '_ ` _ \| |_/ __| *
//* | | | (_) | | | | | |  _\__ \ *
//* |_|  \___/|_| |_| |_|_| |___/ *
//*                               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/romfs/romfs.hpp"

#include <array>

#include "pstore/adt/error_or.hpp"
#include "pstore/support/array_elements.hpp"
#include "pstore/romfs/dirent.hpp"

#include <gmock/gmock.h>

using namespace std::string_literals;
using namespace pstore::romfs;

namespace {

    std::uint8_t const file1[] = {102, 111, 111, 10};
    std::uint8_t const file2[] = {104, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100, 10};
    extern directory const dir0;
    extern directory const dir3;
    constexpr std::time_t foo_mtime = 123;
    constexpr std::time_t hello_mtime = 456;
    std::array<dirent, 3> const dir0_membs = {{
        {".", &dir0},
        {"..", &dir3},
        {"foo", file1, pstore::romfs::stat{sizeof (file1), pstore::romfs::mode_t::file, foo_mtime}},
    }};
    directory const dir0{dir0_membs};
    std::array<dirent, 4> const dir3_membs = {{
        {".", &dir3},
        {"..", &dir3},
        {"dir", &dir0},
        {"hello", file2,
         pstore::romfs::stat{sizeof (file2), pstore::romfs::mode_t::file, hello_mtime}},
    }};
    directory const dir3{dir3_membs};
    directory const * const root = &dir3;


    class RomFs : public ::testing::Test {
    public:
        RomFs ()
                : fs_{root} {}

    protected:
        romfs & fs () noexcept { return fs_; }

        template <typename T>
        void check_for_error (pstore::error_or<T> const & eo, pstore::romfs::error_code actual) {
            ASSERT_FALSE (static_cast<bool> (eo));
            EXPECT_EQ (eo.get_error (), make_error_code (actual));
        }

    private:
        romfs fs_;
    };

} // end anonymous namespace

TEST_F (RomFs, WorkingDirectory) {
    EXPECT_EQ (fs ().getcwd (), "/"s);
    EXPECT_EQ (fs ().chdir ("/."), std::error_code{});
    EXPECT_EQ (fs ().getcwd (), "/"s);

    EXPECT_EQ (fs ().chdir ("hello"), make_error_code (error_code::enotdir));
    EXPECT_EQ (fs ().getcwd (), "/"s) << "Expected no change to the WD after a failed chdir";

    EXPECT_EQ (fs ().chdir ("./dir"), std::error_code{});
    EXPECT_EQ (fs ().getcwd (), "/dir"s);

    EXPECT_EQ (fs ().chdir ("../dir"), std::error_code{});
    EXPECT_EQ (fs ().getcwd (), "/dir"s);

    EXPECT_EQ (fs ().chdir (".."), std::error_code{});
    EXPECT_EQ (fs ().getcwd (), "/"s);

    EXPECT_EQ (fs ().chdir (".."), std::error_code{});
    EXPECT_EQ (fs ().getcwd (), "/"s);
}

TEST_F (RomFs, OpenFile) {
    EXPECT_TRUE (static_cast<bool> (fs ().open ("hello")));
    EXPECT_TRUE (static_cast<bool> (fs ().open ("dir/foo")));
    // Opening a directory "file" is just fine.
    EXPECT_TRUE (static_cast<bool> (fs ().open ("dir")));
    this->check_for_error (fs ().open ("missing"), pstore::romfs::error_code::enoent);
}

TEST_F (RomFs, OpenAndReadFile) {
    pstore::error_or<descriptor> eod = fs ().open ("./hello");
    ASSERT_TRUE (static_cast<bool> (eod));
    descriptor & d = *eod;

    constexpr std::size_t file2_size = pstore::array_elements (file2);
    pstore::romfs::stat const & s = d.stat ();
    EXPECT_EQ (s, (pstore::romfs::stat{file2_size, pstore::romfs::mode_t::file, hello_mtime}));

    std::array<std::uint8_t, file2_size> buffer;
    EXPECT_EQ (d.read (buffer.data (), sizeof (std::uint8_t), buffer.size ()), buffer.size ());
    EXPECT_THAT (buffer, ::testing::ElementsAreArray (file2, file2_size));
    EXPECT_EQ (d.read (buffer.data (), sizeof (std::uint8_t), 1), 0U);

    EXPECT_EQ (d.seek (0, seek_mode::cur), file2_size);
    EXPECT_EQ (d.seek (0, seek_mode::set), std::size_t{0U});
    EXPECT_EQ (d.seek (0, seek_mode::cur), 0);
}

TEST_F (RomFs, OpenDir) {
    this->check_for_error (fs ().opendir ("hello"), pstore::romfs::error_code::enotdir);
    EXPECT_TRUE (static_cast<bool> (fs ().opendir ("/")));

    pstore::error_or<dirent_descriptor> eod = fs ().opendir ("/");
    ASSERT_TRUE (static_cast<bool> (eod));
    auto & value = *eod;
    ASSERT_STREQ (value.read ()->name (), ".");
    ASSERT_STREQ (value.read ()->name (), "..");
    ASSERT_STREQ (value.read ()->name (), "dir");
    ASSERT_STREQ (value.read ()->name (), "hello");
    ASSERT_EQ (value.read (), nullptr);

    value.rewind ();
    ASSERT_STREQ (value.read ()->name (), ".");
}

TEST_F (RomFs, Seek) {
    pstore::error_or<descriptor> eod = fs ().open ("./hello");
    ASSERT_TRUE (static_cast<bool> (eod));
    descriptor & d = *eod;

    std::uint8_t v;
    EXPECT_EQ (d.read (&v, sizeof (v), 1U), 1U);
    EXPECT_EQ (v, 104U);
    pstore::error_or<std::size_t> eos = d.seek (0, seek_mode::set);
    EXPECT_TRUE (static_cast<bool> (eos));
    EXPECT_EQ (*eos, 0U);

    eos = d.seek (1, seek_mode::set);
    EXPECT_TRUE (static_cast<bool> (eos));
    EXPECT_EQ (*eos, 1U);

    eos = d.seek (0, seek_mode::cur);
    EXPECT_TRUE (static_cast<bool> (eos));
    EXPECT_EQ (*eos, 1U);

    eos = d.seek (-2, seek_mode::cur);
    EXPECT_FALSE (static_cast<bool> (eos)) << "Seek past start of file is disallowed";
    EXPECT_EQ (eos.get_error (), make_error_code (pstore::romfs::error_code::einval));

    eos = d.seek (-1, seek_mode::cur);
    EXPECT_TRUE (static_cast<bool> (eos)) << "Seek backwards inside the file should be allowed";
    EXPECT_EQ (*eos, 0U);
}

TEST_F (RomFs, SeekPastEnd) {
    pstore::error_or<descriptor> eod = fs ().open ("./hello");
    ASSERT_TRUE (static_cast<bool> (eod));
    descriptor & d = *eod;

    pstore::error_or<std::size_t> eos = d.seek (3, seek_mode::end);
    EXPECT_TRUE (static_cast<bool> (eos)) << "Seek past EOF should be allowed";
    EXPECT_EQ (*eos, sizeof (file2) + 3U);

    eos = d.seek (0, seek_mode::cur);
    EXPECT_TRUE (static_cast<bool> (eos)) << "Should get current position";
    EXPECT_EQ (*eos, sizeof (file2) + 3U);

    std::uint8_t v;
    EXPECT_EQ (d.read (&v, sizeof (v), 1U), 0U) << "Read from beyond EOF should return 0";
}
