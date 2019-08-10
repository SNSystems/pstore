//*   __ _ _       *
//*  / _(_) | ___  *
//* | |_| | |/ _ \ *
//* |  _| | |  __/ *
//* |_| |_|_|\___| *
//*                *
//===- unittests/os/test_file.cpp -----------------------------------------===//
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
/// \file test_file.cpp
/// \brief Unit tests for the file.hpp (file management) interfaces.

#include "pstore/os/file.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "check_for_error.hpp"

#include "pstore/support/portab.hpp"
#include "pstore/support/small_vector.hpp"

namespace {

    class FileNameTemplate : public ::testing::Test {
    protected:
        struct generator {
            unsigned operator() (unsigned max) { return value++ % max; }
            unsigned value = 0;
        };
        generator rng_;
    };

} // end anonymous namespace

TEST_F (FileNameTemplate, Empty) {
    EXPECT_EQ (pstore::file::details::name_from_template ("", rng_), std::string{""});
}
TEST_F (FileNameTemplate, NoTrailingX) {
    EXPECT_EQ (pstore::file::details::name_from_template ("A", rng_), "A");
}
TEST_F (FileNameTemplate, TrailingXOnly) {
    EXPECT_EQ (pstore::file::details::name_from_template ("X", rng_), "a");
}
TEST_F (FileNameTemplate, CharacterWithOneTrailingX) {
    EXPECT_EQ (pstore::file::details::name_from_template ("AX", rng_), "Aa");
}
TEST_F (FileNameTemplate, CharacterWithMultipleTrailingX) {
    EXPECT_EQ (pstore::file::details::name_from_template ("AXXXXX", rng_), "Aabcde");
}
TEST_F (FileNameTemplate, CharacterWithLeadingAndMultipleTrailingX) {
    EXPECT_EQ (pstore::file::details::name_from_template ("X!XXXXX", rng_), "X!abcde");
}



namespace {

    // A file class which is used to mock the lock() and unlock() methods.
    class mock_file final : public pstore::file::file_base {
    public:
        MOCK_METHOD0 (close, void());

        // is_open() and is_writable() are noexcept functions which gmock cannot currently directly
        // intercept.
        MOCK_CONST_METHOD0 (is_open_hook, bool());
        bool is_open () const noexcept override { return this->is_open_hook (); }

        MOCK_CONST_METHOD0 (is_writable_hook, bool());
        bool is_writable () const noexcept override { return this->is_writable_hook (); }

        MOCK_CONST_METHOD0 (path, std::string ());
        MOCK_METHOD1 (seek, void(std::uint64_t));
        MOCK_METHOD0 (tell, std::uint64_t ());
        MOCK_METHOD2 (read_buffer, std::size_t (pstore::gsl::not_null<void *>, std::size_t));
        MOCK_METHOD2 (write_buffer, void(pstore::gsl::not_null<void const *>, std::size_t));
        MOCK_METHOD0 (size, std::uint64_t ());
        MOCK_METHOD1 (truncate, void(std::uint64_t));
        MOCK_CONST_METHOD0 (latest_time, std::time_t ());

        MOCK_METHOD4 (lock, bool(std::uint64_t, std::size_t, lock_kind, blocking_mode));
        MOCK_METHOD2 (unlock, void(std::uint64_t, std::size_t));
    };

} // end anonymous namespace

TEST (RangeLock, InitialState) {
    pstore::file::range_lock lock;
    EXPECT_EQ (nullptr, lock.file ());
    EXPECT_EQ (UINT64_C (0), lock.offset ());
    EXPECT_EQ (std::size_t{0}, lock.size ());
    EXPECT_EQ (mock_file::lock_kind::shared_read, lock.kind ());
    EXPECT_FALSE (lock.is_locked ());
}

TEST (RangeLock, ExplicitInitialization) {
    using ::testing::_;
    mock_file file;

    // These expectations are present to suppress a warning from clang about the member functions
    // being unused...
    EXPECT_CALL (file, close ()).Times (0);
    EXPECT_CALL (file, is_open_hook ()).Times (0);
    EXPECT_CALL (file, is_writable_hook ()).Times (0);
    EXPECT_CALL (file, path ()).Times (0);
    EXPECT_CALL (file, seek (_)).Times (0);
    EXPECT_CALL (file, tell ()).Times (0);
    EXPECT_CALL (file, read_buffer (_, _)).Times (0);
    EXPECT_CALL (file, write_buffer (_, _)).Times (0);
    EXPECT_CALL (file, size ()).Times (0);
    EXPECT_CALL (file, truncate (_)).Times (0);
    EXPECT_CALL (file, latest_time ()).Times (0);

    // Now the real test.
    EXPECT_CALL (file, lock (_, _, _, _)).Times (0);
    EXPECT_CALL (file, unlock (_, _)).Times (0);

    pstore::file::range_lock lock (&file,
                                   UINT64_C (5),   // offset
                                   std::size_t{7}, // size
                                   mock_file::lock_kind::exclusive_write);
    EXPECT_EQ (&file, lock.file ());
    EXPECT_EQ (UINT64_C (5), lock.offset ());
    EXPECT_EQ (std::size_t{7}, lock.size ());
    EXPECT_EQ (mock_file::lock_kind::exclusive_write, lock.kind ());
    EXPECT_FALSE (lock.is_locked ());
}

TEST (RangeLock, MoveConstruct) {
    mock_file file;

    using ::testing::_;
    EXPECT_CALL (file, lock (_, _, _, _)).Times (0);
    EXPECT_CALL (file, unlock (_, _)).Times (0);

    pstore::file::range_lock lock1 (&file,
                                    UINT64_C (13),   // offset
                                    std::size_t{17}, // size
                                    mock_file::lock_kind::exclusive_write);
    pstore::file::range_lock lock2 = std::move (lock1);

    // Check that lock1 was "destroyed" by the move
    EXPECT_EQ (nullptr, lock1.file ());
    EXPECT_FALSE (lock1.is_locked ());

    // Check that lock2 matches the state of lock1 before the move
    EXPECT_EQ (&file, lock2.file ());
    EXPECT_EQ (UINT64_C (13), lock2.offset ());
    EXPECT_EQ (std::size_t{17}, lock2.size ());
    EXPECT_EQ (mock_file::lock_kind::exclusive_write, lock2.kind ());
    EXPECT_FALSE (lock2.is_locked ());
}

TEST (RangeLock, MoveAssign) {
    mock_file file1;
    mock_file file2;

    using ::testing::_;
    using ::testing::Expectation;
    using ::testing::Return;

    Expectation file1_lock = EXPECT_CALL (file1, lock (UINT64_C (13),   // offset
                                                       std::size_t{17}, // size
                                                       mock_file::lock_kind::exclusive_write,
                                                       mock_file::blocking_mode::blocking))
                                 .WillOnce (Return (true));

    Expectation file2_lock = EXPECT_CALL (file2, lock (UINT64_C (19),   // offset
                                                       std::size_t{23}, // size
                                                       mock_file::lock_kind::shared_read,
                                                       mock_file::blocking_mode::blocking))
                                 .After (file1_lock)
                                 .WillOnce (Return (true));

    Expectation file2_unlock =
        EXPECT_CALL (file2, unlock (UINT64_C (19), std::size_t{23})).After (file2_lock);

    Expectation file1_unlock = EXPECT_CALL (file1, unlock (UINT64_C (13), std::size_t{17}))
                                   .Times (1)
                                   .After (file1_lock, file2_unlock);



    pstore::file::range_lock source_lock (&file1,
                                          UINT64_C (13),   // offset
                                          std::size_t{17}, // size
                                          mock_file::lock_kind::exclusive_write);
    pstore::file::range_lock target_lock (&file2,
                                          UINT64_C (19),   // offset
                                          std::size_t{23}, // size
                                          mock_file::lock_kind::shared_read);

    // Lock both the source and target of the assignment, just to be tricky.
    source_lock.lock ();
    target_lock.lock ();
    target_lock = std::move (source_lock);

    // Check that lock1 was "destroyed" by the move
    EXPECT_EQ (nullptr, source_lock.file ())
        << "The file associated with the source of a move assignment should be null";
    EXPECT_FALSE (source_lock.is_locked ())
        << "The source of a move assignment should not be locked";

    // Check that lock2 matches the state of lock1 before the move
    EXPECT_EQ (&file1, target_lock.file ())
        << "The file associated with the target of the move is wrong";
    EXPECT_EQ (UINT64_C (13), target_lock.offset ())
        << "The offset of the move target range_lock is wrong";
    EXPECT_EQ (std::size_t{17}, target_lock.size ())
        << "The size of the move target range_lock is wrong";
    EXPECT_EQ (mock_file::lock_kind::exclusive_write, target_lock.kind ());
    EXPECT_TRUE (target_lock.is_locked ())
        << "Expected the target of the move operation to be locked";

    target_lock.unlock ();
}

TEST (RangeLock, LockUnlock) {
    mock_file file;

    // Set up the test expectations.
    using ::testing::_;
    using ::testing::Expectation;
    using ::testing::Return;

    Expectation file_lock = EXPECT_CALL (file, lock (UINT64_C (5), std::size_t{7},
                                                     mock_file::lock_kind::exclusive_write,
                                                     mock_file::blocking_mode::blocking))
                                .WillOnce (Return (true));
    EXPECT_CALL (file, unlock (UINT64_C (5), std::size_t{7})).Times (1).After (file_lock);

    pstore::file::range_lock lock (&file,
                                   UINT64_C (5),   // offset
                                   std::size_t{7}, // size
                                   mock_file::lock_kind::exclusive_write);
    lock.lock ();
    EXPECT_TRUE (lock.is_locked ());
    lock.unlock ();
    EXPECT_FALSE (lock.is_locked ());
}

TEST (RangeLock, TryLockSucceeds) {
    mock_file file;

    // Set up the test expectations.
    using ::testing::_;
    using ::testing::Return;
    EXPECT_CALL (file, lock (UINT64_C (5), std::size_t{7}, mock_file::lock_kind::exclusive_write,
                             mock_file::blocking_mode::non_blocking))
        .WillOnce (Return (true));
    EXPECT_CALL (file, unlock (UINT64_C (5), std::size_t{7})).Times (1);

    pstore::file::range_lock lock (&file,
                                   UINT64_C (5),   // offset
                                   std::size_t{7}, // size
                                   mock_file::lock_kind::exclusive_write);
    bool locked = lock.try_lock ();
    EXPECT_TRUE (locked);
    EXPECT_TRUE (lock.is_locked ());
    lock.unlock ();
}

TEST (RangeLock, TryLockFails) {
    mock_file file;

    // Set up the test expectations.
    using ::testing::_;
    using ::testing::Return;
    EXPECT_CALL (file, lock (UINT64_C (5), std::size_t{7}, mock_file::lock_kind::exclusive_write,
                             mock_file::blocking_mode::non_blocking))
        .WillOnce (Return (false));
    EXPECT_CALL (file, unlock (_, _)).Times (0);

    pstore::file::range_lock lock (&file,
                                   UINT64_C (5),   // offset
                                   std::size_t{7}, // size
                                   mock_file::lock_kind::exclusive_write);
    EXPECT_FALSE (lock.try_lock ());
    EXPECT_FALSE (lock.is_locked ());
    lock.unlock ();
}

TEST (RangeLock, LockWithNoFile) {
    pstore::file::range_lock lock;
    EXPECT_FALSE (lock.lock ());
    EXPECT_FALSE (lock.is_locked ());
    EXPECT_FALSE (lock.try_lock ());
    lock.unlock ();
    EXPECT_FALSE (lock.is_locked ());
}

#if PSTORE_EXCEPTIONS
TEST (RangeLock, ErrorWithLockHeld) {
    mock_file file;

    using ::testing::_;
    EXPECT_CALL (file, lock (_, _, _, _)).WillOnce (::testing::Return (true));
    EXPECT_CALL (file, unlock (_, _)).Times (1);

    class custom_exception : public std::exception {};

    try {
        constexpr auto offset = std::uint64_t{5};
        constexpr auto size = std::size_t{7};
        pstore::file::range_lock lock (&file, offset, size, mock_file::lock_kind::exclusive_write);
        lock.lock ();
        // Throw with the lock held.
        throw custom_exception{};
    } catch (custom_exception const &) {
    } catch (...) {
        GTEST_FAIL () << "An unexpected exception was raised.";
    }
}
#endif // PSTORE_EXCEPTIONS


namespace {

    class DeleterFixture : public ::testing::Test {
    public:
        void unlink (std::string const & name);

    protected:
        using path_list = std::vector<std::string>;
        // TODO: surely I could do this with Google Mock?
        path_list unlinked_files_;
    };

    void DeleterFixture::unlink (std::string const & name) { unlinked_files_.push_back (name); }


    class test_file_deleter final : public pstore::file::deleter_base {
    public:
        test_file_deleter (std::string const & path, DeleterFixture * const fixture)
                : deleter_base (path, unlinker (fixture)) {}

    private:
        static auto unlinker (DeleterFixture * const fixture) -> deleter_base::unlink_proc {

            return [=](std::string const & name) -> void { fixture->unlink (name); };
        }
    };

} // end anonymous namespace


TEST_F (DeleterFixture, UnlinkCallsPlatformUnlinkWithOriginalPath) {
    {
        test_file_deleter d ("path", this);
        d.unlink ();
    }
    EXPECT_THAT (path_list{{"path"}}, ::testing::ContainerEq (unlinked_files_));
}

TEST_F (DeleterFixture, UnlinkDoesNotCallPlatformUnlinkAfterRelease) {
    {
        test_file_deleter d ("path", this);
        d.release ();
        d.unlink ();
    }
    EXPECT_THAT (path_list{}, ::testing::ContainerEq (unlinked_files_));
}

TEST_F (DeleterFixture, DestructorCallsPlatformUnlinkWithOriginalPath) {
    { test_file_deleter d ("path", this); }
    EXPECT_THAT (path_list{{"path"}}, ::testing::ContainerEq (unlinked_files_));
}

TEST_F (DeleterFixture, DestructorDoesNotCallPlatformUnlinkAfterRelease) {
    {
        test_file_deleter d ("path", this);
        d.release ();
    }
    EXPECT_THAT (path_list{}, ::testing::ContainerEq (unlinked_files_));
}


namespace {

    class TemporaryFileFixture : public ::testing::Test {
    protected:
        TemporaryFileFixture ()
                : deleter_{std::make_shared<pstore::file::file_handle> ()} {
            deleter_->open (pstore::file::file_handle::temporary{});
        }

        std::shared_ptr<pstore::file::file_handle> deleter_;
    };

} // end anonymous namespace

TEST_F (TemporaryFileFixture, CheckTemporaryFileIsDeleted) {
    std::string path = deleter_->path ();
    deleter_.reset ();
    EXPECT_FALSE (pstore::file::exists (path));
}


namespace {

    class MemoryFile : public ::testing::Test {
    public:
        void SetUp () override {}
        void TearDown () override {}

        using buffer = std::shared_ptr<std::uint8_t>;

        static buffer make_buffer (std::size_t elements) {
            return buffer (new std::uint8_t[elements], [](std::uint8_t * p) { delete[] p; });
        }
        static buffer make_buffer (std::string const & str) {
            auto result = make_buffer (str.length ());
            std::copy (std::begin (str), std::end (str), result.get ());
            return result;
        }
    };

} // end anonymous namespace

TEST_F (MemoryFile, FileIsInitiallyEmpty) {
    constexpr std::size_t elements = 13;
    pstore::file::in_memory mf (MemoryFile::make_buffer (elements), elements);
    EXPECT_EQ (0U, mf.tell ()) << "Expected the initial file offset to be 0";
    EXPECT_EQ (0U, mf.size ()) << "Expected the file to be initially empty";
}

TEST_F (MemoryFile, ReadFileWithInitialContents) {
    constexpr std::size_t elements = 11;
    char const source_string[elements + 1]{"Hello World"};
    ASSERT_EQ (elements, std::strlen (source_string))
        << "Expected buffer length to be " << elements;
    pstore::file::in_memory mf (MemoryFile::make_buffer (source_string), elements, elements);

    EXPECT_EQ (0U, mf.tell ()) << "Expected the initial file offset to be 0";
    EXPECT_EQ (elements, mf.size ()) << "Expected the file to be the same size as the string";

    auto out = MemoryFile::make_buffer (elements);
    std::size_t const actual_read = mf.read_buffer (out.get (), elements);
    ASSERT_EQ (elements, actual_read);
    EXPECT_TRUE (std::equal (out.get (), out.get () + elements, source_string));
    EXPECT_EQ (elements, mf.tell ());
}

TEST_F (MemoryFile, ReadPastEndOfFileWithInitialContents) {
    constexpr std::size_t elements = 5;
    char const source_string[elements + 1]{"Hello"};
    ASSERT_EQ (elements, std::strlen (source_string))
        << "Expected source length to be " << elements;
    pstore::file::in_memory mf (MemoryFile::make_buffer (source_string), elements, elements);

    constexpr auto out_elements = std::size_t{7};
    auto out = MemoryFile::make_buffer (out_elements);
    std::size_t const actual_read = mf.read_buffer (out.get (), out_elements);
    ASSERT_EQ (elements, actual_read);
    EXPECT_TRUE (std::equal (out.get (), out.get () + elements, "Hello\0"));
    EXPECT_EQ (elements, mf.tell ());
}

TEST_F (MemoryFile, WriteToInitiallyEmptyFile) {
    constexpr std::size_t elements = 5;
    auto buffer = MemoryFile::make_buffer (elements);
    std::fill (buffer.get (), buffer.get () + elements, std::uint8_t{0});
    pstore::file::in_memory mf (buffer, elements);

    char const * source = "Hello";
    ASSERT_GE (sizeof (source), elements);
    mf.write_buffer (source, elements);
    EXPECT_EQ (5U, mf.tell ());
    EXPECT_EQ (5U, mf.size ());

    EXPECT_TRUE (std::equal (buffer.get (), buffer.get () + elements, source));
}

TEST_F (MemoryFile, CrazyWriteSize) {
    constexpr std::size_t elements = 10;
    auto buffer = MemoryFile::make_buffer (elements);
    std::fill (buffer.get (), buffer.get () + elements, std::uint8_t{0});
    pstore::file::in_memory mf (buffer, elements);

    mf.write (std::numeric_limits<std::uint32_t>::max ());
    mf.seek (4);
    auto const length = std::numeric_limits<std::size_t>::max () - std::size_t{2};
    char const * source = "Hello";
    check_for_error ([&] { mf.write_buffer (source, length); }, std::errc::invalid_argument);
}

TEST_F (MemoryFile, Seek) {
    constexpr std::size_t elements = 5;
    char const source_string[elements + 1]{"abcde"};
    ASSERT_EQ (elements, std::strlen (source_string))
        << "Expected source length to be " << elements;
    pstore::file::in_memory mf (MemoryFile::make_buffer (source_string), elements, elements);

    // Seek to position 1. Check tell() and read ().
    mf.seek (1);
    EXPECT_EQ (1U, mf.tell ());

    {
        auto out1 = MemoryFile::make_buffer (4);
        std::fill (out1.get (), out1.get () + 4, std::uint8_t{0});
        std::size_t const actual_read = mf.read_buffer (out1.get (), 4);
        ASSERT_EQ (4U, actual_read);
        EXPECT_TRUE (std::equal (out1.get (), out1.get () + 4, "bcde"));
    }
    // Seek to 4. Check tell () and read past EOF.
    mf.seek (4);
    EXPECT_EQ (4U, mf.tell ());
    {

        auto out2 = MemoryFile::make_buffer (2);
        std::fill (out2.get (), out2.get () + 2, '\x7F');
        std::size_t const actual_read = mf.read_buffer (out2.get (), 2);
        ASSERT_EQ (1U, actual_read);
        EXPECT_TRUE (std::equal (out2.get (), out2.get () + 2, "e\x7F"));
    }

    // Seek past EOF
    check_for_error ([&mf]() { mf.seek (127); }, std::errc::invalid_argument);
    EXPECT_EQ (5U, mf.tell ());
}

TEST_F (MemoryFile, Truncate) {
    constexpr std::size_t elements = 5;
    char const source_string[elements + 1]{"abcde"};
    ASSERT_EQ (elements, std::strlen (source_string))
        << "Expected source length to be " << elements;
    pstore::file::in_memory mf (MemoryFile::make_buffer (source_string), elements, elements);

    mf.truncate (0);
    EXPECT_EQ (0U, mf.size ());
    EXPECT_EQ (0U, mf.tell ());

    mf.truncate (5);
    EXPECT_EQ (5U, mf.size ());
    EXPECT_EQ (0U, mf.tell ());
    mf.seek (5);
    EXPECT_EQ (5U, mf.tell ());

    mf.truncate (0);
    EXPECT_EQ (0U, mf.size ());
    EXPECT_EQ (0U, mf.tell ());
    check_for_error ([&mf]() { mf.truncate (6); }, std::errc::invalid_argument);
    EXPECT_EQ (0U, mf.tell ());


    mf.truncate (5);
    mf.seek (5);
    mf.truncate (4);
    EXPECT_EQ (4U, mf.tell ());

    mf.truncate (5);
    EXPECT_EQ (4U, mf.tell ());
    mf.seek (3);
    mf.truncate (4);
    EXPECT_EQ (3U, mf.tell ());
}



namespace {

    class NativeFile : public ::testing::Test {
    public:
        NativeFile () {
            using namespace pstore::file;
            file_.open (file_handle::temporary{}, file_handle::get_temporary_directory ());
        }
        ~NativeFile () {
            PSTORE_NO_EX_ESCAPE ({ file_.close (); });
        }

    protected:
        pstore::file::file_handle file_;
    };

} // end anonymous namespace

TEST_F (NativeFile, ReadEmptyFile) {
    char c[2];
    EXPECT_EQ (0U, file_.read_span (pstore::gsl::make_span (c)));

    check_for_error (
        [this]() {
            long l;
            file_.read (&l);
        },
        pstore::error_code::did_not_read_number_of_bytes_requested);

    EXPECT_EQ (0U, file_.tell ());
}

TEST_F (NativeFile, ReadTinyFile) {
    file_.write ('a');
    file_.seek (0U);

    char c[2];
    EXPECT_EQ (sizeof (char), file_.read_span (::pstore::gsl::make_span (c)));

    check_for_error (
        [this]() {
            file_.seek (0U);
            long l;
            file_.read (&l);
        },
        pstore::error_code::did_not_read_number_of_bytes_requested);

    // I expect the file's contents to have been read by the call to file_read<>() and hence the
    // file position should have changed.
    EXPECT_EQ (sizeof (char), file_.tell ());

    // Now check the success condition: where the file just contains enough data.
    file_.seek (0U);
    char c2[1];
    EXPECT_EQ (sizeof (c2), file_.read_span (::pstore::gsl::make_span (c2)));
}



#ifdef _WIN32

namespace {

    class EnvironmentSaveFixture : public ::testing::Test {
    public:
        // Save the environment variables tht the test may replace.
        void SetUp () final;
        // Restore the environment variable values.
        void TearDown () final;

    protected:
        void set_temp_path (std::wstring const & path);
        static std::wstring getenv (std::wstring const & key);

    private:
        std::map<std::wstring, std::wstring> env_;
    };

    // SetUp
    // ~~~~~
    void EnvironmentSaveFixture::SetUp () {
        env_.clear ();
        std::array<std::wstring, 3> const keys{{L"TMP", L"TEMP", L"USERPROFILE"}};
        for (auto const & key : keys) {
            env_[key] = this->getenv (key);
        }
    }

    // TearDown
    // ~~~~~~~~
    void EnvironmentSaveFixture::TearDown () {
        for (auto const & kvp : env_) {
            ::SetEnvironmentVariableW (kvp.first.c_str (), kvp.second.c_str ());
        }
        env_.clear ();
    }

    // set_temp_path
    // ~~~~~~~~~~~~~
    void EnvironmentSaveFixture::set_temp_path (std::wstring const & path) {
        // Note that the three environment variables that I'm setting here
        // are the three that Microsoft documents as being used by
        // GetTempPathW().

        ::SetEnvironmentVariableW (L"TMP", path.c_str ());
        ::SetEnvironmentVariableW (L"TEMP", nullptr);
        ::SetEnvironmentVariableW (L"USERPROFILE", nullptr);
    }

    // getenv
    // ~~~~~~
    std::wstring EnvironmentSaveFixture::getenv (std::wstring const & key) {
        pstore::small_vector<wchar_t> buffer;
        DWORD sz = ::GetEnvironmentVariableW (key.c_str (), buffer.data (),
                                              static_cast<DWORD> (buffer.size ()));
        if (sz == 0) {
            return {};
        } else if (sz > buffer.size ()) {
            buffer.resize (sz);
            assert (buffer.size () == sz);
            sz = ::GetEnvironmentVariableW (key.c_str (), buffer.data (), sz);
            if (sz == 0) {
                return {};
            }
        }

        assert (buffer.data ()[sz] == L'\0');
        return {buffer.data (), sz};
    }

} // end anonymous namespace

#    ifdef PSTORE_EXCEPTIONS

TEST_F (EnvironmentSaveFixture, TaintedEnvironmentBadUTF16) {
    // Cook up a temporary path which will be on the "system" drive and contain
    // bad UTF-16; the attempt to create the file should fail with a reasonable
    // error.

    wchar_t const * bad_utf16 = L"\\\xD800\x0041";
    std::wstring const path = this->getenv (L"SystemDrive") + bad_utf16;
    std::string const path_utf8 = pstore::utf::win32::to8 (path);

    this->set_temp_path (path);
    try {
        // open() is expected to throw an exception when passed bad UTF-16.
        pstore::file::file_handle f;
        f.open (pstore::file::file_handle::temporary{});
        ASSERT_TRUE (false) << "shouldn't be reachable";
    } catch (std::exception const & ex) {
        // Check that the exception message contains the file path. We've had
        // to convert the Windows-style UTF-16 string to UTF-8 to that it can be
        // carried by std::exception.
        std::string const & message = ex.what ();
        EXPECT_THAT (message, ::testing::HasSubstr (path_utf8))
            << "exception should contain the file path";
    } catch (...) {
        ASSERT_TRUE (false) << "shouldn't be reachable";
    }
}

TEST_F (EnvironmentSaveFixture, TaintedEnvironmentInvalidPath) {
    // Cook up a temporary path which points to a reasonable, but missing path.
    std::wstring path = this->getenv (L"SystemDrive") + L"\\aaa\\aaa\\aaa\\aaa\\";

    std::string const path_utf8 = pstore::utf::win32::to8 (path);
    ASSERT_FALSE (pstore::file::exists (path_utf8))
        << "I really didn't expect the path \"" << path_utf8 << "\" to exist!";
    this->set_temp_path (path);

    // Now check that the exception message contains the file path. We've had
    // to convert the Windows-style UTF-16 string to UTF-8 to that it can be
    // carried by std::exception.

    try {
        // We expect that calls to create() will throw an exception. The file name
        // is too long for Windows's little mind.
        pstore::file::file_handle f;
        f.open (pstore::file::file_handle::temporary{});
        ASSERT_TRUE (false) << "shouldn't be reachable";
    } catch (std::exception const & ex) {
        std::string const & message = ex.what ();
        EXPECT_THAT (message, ::testing::HasSubstr (path_utf8))
            << "exception should contain the file path";
    } catch (...) {
        ASSERT_TRUE (false) << "shouldn't be reachable";
    }
}
#    endif // PSTORE_EXCEPTIONS

#endif //_WIN32
