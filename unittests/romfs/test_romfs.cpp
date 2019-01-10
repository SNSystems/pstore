//*                       __      *
//*  _ __ ___  _ __ ___  / _|___  *
//* | '__/ _ \| '_ ` _ \| |_/ __| *
//* | | | (_) | | | | | |  _\__ \ *
//* |_|  \___/|_| |_| |_|_| |___/ *
//*                               *
//===- unittests/romfs/test_romfs.cpp -------------------------------------===//
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
#include "pstore/romfs/romfs.hpp"
#include <array>
#include "pstore/support/array_elements.hpp"
#include "pstore/support/make_unique.hpp"
#include "pstore/romfs/dirent.hpp"

#include "gmock/gmock.h"

using namespace pstore::romfs;

namespace {

    std::uint8_t const file1[] = {102, 111, 111, 10};
    std::uint8_t const file2[] = {104, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100, 10};
    extern directory const dir0;
    extern directory const dir3;
    std::array<dirent, 3> const dir0_membs = {{
        {".", &dir0},
        {"..", &dir3},
        {"foo", file1, sizeof (file1), 0},
    }};
    directory const dir0{dir0_membs};
    std::array<dirent, 4> const dir3_membs = {{
        {".", &dir3},
        {"..", &dir3},
        {"dir", &dir0},
        {"hello", file2, sizeof (file2), 0},
    }};
    directory const dir3{dir3_membs};
    directory const * const root = &dir3;


    class RomFs : public ::testing::Test {
    public:
        RomFs ()
                : fs_{root} {}

    protected:
        romfs fs_;

        template <typename T>
        void check_for_error (error_or<T> const & eo, pstore::romfs::error_code actual) {
            ASSERT_TRUE (eo.has_error ());
            EXPECT_EQ (eo.get_error (), std::make_error_code (actual));
        }
    };

} // end anonymous namespace

namespace pstore {
    namespace romfs {

        template <typename T>
        std::ostream & operator<< (std::ostream & os, error_or<T> const & eor) {
            if (eor.has_error ()) {
                os << "Error:" << eor.get_error ().message ();
            } else {
                os << "Value:" << eor.get_value ();
            }
            return os;
        }

    } // end namespace romfs
} // end namespace pstore

TEST_F (RomFs, WorkingDirectory) {
    EXPECT_EQ (fs_.getcwd (), error_or<std::string> (in_place, "/"));
    EXPECT_EQ (fs_.chdir ("/."), std::error_code{});
    EXPECT_EQ (fs_.getcwd (), error_or<std::string> (in_place, "/"));

    EXPECT_EQ (fs_.chdir ("hello"), std::make_error_code (error_code::enotdir));
    EXPECT_EQ (fs_.getcwd (), error_or<std::string> (in_place, "/"))
        << "Expected no change to the WD after a failed chdir";

    EXPECT_EQ (fs_.chdir ("./dir"), std::error_code{});
    EXPECT_EQ (fs_.getcwd (), error_or<std::string> (in_place, "/dir"));

    EXPECT_EQ (fs_.chdir ("../dir"), std::error_code{});
    EXPECT_EQ (fs_.getcwd (), error_or<std::string> (in_place, "/dir"));

    EXPECT_EQ (fs_.chdir (".."), std::error_code{});
    EXPECT_EQ (fs_.getcwd (), error_or<std::string> (in_place, "/"));

    EXPECT_EQ (fs_.chdir (".."), std::error_code{});
    EXPECT_EQ (fs_.getcwd (), error_or<std::string> (in_place, "/"));
}

TEST_F (RomFs, OpenFile) {
    EXPECT_FALSE (fs_.open ("hello").has_error ());
    EXPECT_FALSE (fs_.open ("dir/foo").has_error ());
    // Opening a directory "file" is just fine.
    EXPECT_FALSE (fs_.open ("dir").has_error ());
    this->check_for_error (fs_.open ("missing"), pstore::romfs::error_code::enoent);
}

TEST_F (RomFs, OpenAndReadFile) {
    error_or<descriptor> eod = fs_.open ("./hello");
    ASSERT_FALSE (eod.has_error ());
    descriptor & d = eod.get_value ();

    constexpr std::size_t file2_size = pstore::array_elements (file2);
    pstore::romfs::stat const & s = d.stat ();
    EXPECT_EQ (s, (pstore::romfs::stat{file2_size, 0}));

    std::array<std::uint8_t, file2_size> buffer;
    EXPECT_EQ (d.read (buffer.data (), sizeof (std::uint8_t), buffer.size ()), buffer.size ());
    EXPECT_THAT (buffer, ::testing::ElementsAreArray (file2, file2_size));
    EXPECT_EQ (d.read (buffer.data (), sizeof (std::uint8_t), 1), 0U);

    EXPECT_EQ (d.seek (0, SEEK_CUR), file2_size);
    EXPECT_EQ (d.seek (0, SEEK_SET), (error_or<std::size_t>{0U}));
    EXPECT_EQ (d.seek (0, SEEK_CUR), 0);
}

TEST_F (RomFs, OpenDir) {
    this->check_for_error (fs_.opendir ("hello"), pstore::romfs::error_code::enotdir);
    EXPECT_FALSE (fs_.opendir ("/").has_error ());

    error_or<dirent_descriptor> eod = fs_.opendir ("/");
    ASSERT_FALSE (eod.has_error ());
    auto & value = eod.get_value ();
    ASSERT_STREQ (value.read ()->name (), ".");
    ASSERT_STREQ (value.read ()->name (), "..");
    ASSERT_STREQ (value.read ()->name (), "dir");
    ASSERT_STREQ (value.read ()->name (), "hello");
    ASSERT_EQ (value.read (), nullptr);

    value.rewind ();
    ASSERT_STREQ (value.read ()->name (), ".");
}

TEST_F (RomFs, Seek) {
    error_or<descriptor> eod = fs_.open ("./hello");
    ASSERT_FALSE (eod.has_error ());
    descriptor & d = eod.get_value ();

    std::uint8_t v;
    EXPECT_EQ (d.read (&v, sizeof (v), 1U), 1U);
    EXPECT_EQ (v, 104U);
    error_or<std::size_t> eos = d.seek (0, SEEK_SET);
    EXPECT_FALSE (eos.has_error ());
    EXPECT_EQ (eos.get_value (), 0U);

    eos = d.seek (1, SEEK_SET);
    EXPECT_FALSE (eos.has_error ());
    EXPECT_EQ (eos.get_value (), 1U);

    eos = d.seek (0, SEEK_CUR);
    EXPECT_FALSE (eos.has_error ());
    EXPECT_EQ (eos.get_value (), 1U);

    eos = d.seek (-2, SEEK_CUR);
    EXPECT_TRUE (eos.has_error ()) << "Seek past start of file is disallowed";
    EXPECT_EQ (eos.get_error (), std::make_error_code (pstore::romfs::error_code::einval));

    eos = d.seek (-1, SEEK_CUR);
    EXPECT_FALSE (eos.has_error ());
}

TEST_F (RomFs, SeekPastEnd) {
    error_or<descriptor> eod = fs_.open ("./hello");
    ASSERT_FALSE (eod.has_error ());
    descriptor & d = eod.get_value ();

    error_or<std::size_t> eos = d.seek (3, SEEK_END);
    EXPECT_FALSE (eos.has_error ()) << "Seek past EOF should be allowed";
    EXPECT_EQ (eos.get_value (), sizeof (file2) + 3U);

    eos = d.seek (0, SEEK_CUR);
    EXPECT_FALSE (eos.has_error ()) << "Should get current position";
    EXPECT_EQ (eos.get_value (), sizeof (file2) + 3U);

    std::uint8_t v;
    EXPECT_EQ (d.read (&v, sizeof (v), 1U), 0U) << "Read from beyond EOF should return 0";
}
