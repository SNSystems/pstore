//*  _               _        _                              *
//* | |__   __ _ ___(_) ___  | | ___   __ _  __ _  ___ _ __  *
//* | '_ \ / _` / __| |/ __| | |/ _ \ / _` |/ _` |/ _ \ '__| *
//* | |_) | (_| \__ \ | (__  | | (_) | (_| | (_| |  __/ |    *
//* |_.__/ \__,_|___/_|\___| |_|\___/ \__, |\__, |\___|_|    *
//*                                   |___/ |___/            *
//===- unittests/pstore/test_basic_logger.cpp -----------------------------===//
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
/// \file test_basic_logger.cpp

#include "pstore_support/logging.hpp"

#include <cstring>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "pstore_support/error.hpp"
#include "pstore_support/utf.hpp"
#include "pstore_support/thread.hpp"

namespace {
    //***************************************
    //*   t i m e _ z o n e _ s e t t e r   *
    // **************************************
    class time_zone_setter {
    public:
        explicit time_zone_setter (char const * tz);
        ~time_zone_setter () noexcept;

    private:
        std::unique_ptr<std::string> tz_value () const;
        void set (char const * tz);
        static void setenv (char const * var, char const * value);
        std::unique_ptr<std::string> old_;
    };

    // (ctor)
    // ~~~~~~
    time_zone_setter::time_zone_setter (char const * tz)
            : old_ (tz_value ()) {
        this->set (tz);
    }

    // (dtor)
    // ~~~~~~
    time_zone_setter::~time_zone_setter () noexcept {
        if (std::string const * const s = old_.get ()) {
            time_zone_setter::setenv ("TZ", s->c_str ());
        } else {
#ifdef _WIN32
            ::_putenv ("TZ=");
#else
            ::unsetenv ("TZ");
#endif
        }
    }

    // tz_value
    // ~~~~~~~~
    std::unique_ptr<std::string> time_zone_setter::tz_value () const {
        char const * old = std::getenv ("TZ");
        return std::unique_ptr<std::string> (old != nullptr ? new std::string{old} : nullptr);
    }

    // set
    // ~~~
    void time_zone_setter::set (char const * tz) {
        assert (tz != nullptr);
        this->setenv ("TZ", tz);
    }

    // setenv
    // ~~~~~~
    void time_zone_setter::setenv (char const * name, char const * value) {
        assert (name != nullptr && value != nullptr);
#ifdef _WIN32
        using ::pstore::utf::win32::to16;
        if (errno_t const err = ::_wputenv_s (to16 (name).c_str (), to16 (value).c_str ())) {
            raise (pstore::errno_erc{err}, "_wputenv_s");
        }
        ::_tzset ();
#else
        errno = 0;
        if (::setenv (name, value, 1 /*overwrite*/) != 0) {
            raise (pstore::errno_erc{errno}, "setenv");
        }
        ::tzset ();
#endif
    }


    //***************************************************
    //*   B a s i c L o g g e r T i m e F i x t u r e   *
    //***************************************************
    struct BasicLoggerTimeFixture : public ::testing::Test {
        std::array <char, ::pstore::logging::basic_logger::time_buffer_size> buffer_;
        static constexpr unsigned const sign_index_ = 19;

        // If the time zone offset is 0, 0he standard library could legimately describe that
        // as either +0000 or -0000. Canonicalize here (to -0000).
        void canonicalize_sign ();
    };

    // canonicalize_sign
    // ~~~~~~~~~~~~~~~~~
    void BasicLoggerTimeFixture::canonicalize_sign () {
        static_assert (::pstore::logging::basic_logger::time_buffer_size >= sign_index_,
                       "sign index is too large for time_buffer");
        ASSERT_EQ ('\0', buffer_[::pstore::logging::basic_logger::time_buffer_size - 1]);
        if (std::strcmp (&buffer_[sign_index_], "+0000") == 0) {
            buffer_[sign_index_] = '-';
        }
    }
}

TEST_F (BasicLoggerTimeFixture, EpochInUTC) {
    time_zone_setter tzs ("UTC0");
    std::size_t const r =
        ::pstore::logging::basic_logger::time_string (std::time_t{0}, ::pstore::gsl2::make_span (buffer_));
    EXPECT_EQ (std::size_t{24}, r);
    EXPECT_EQ ('\0', buffer_[24]);
    this->canonicalize_sign ();
    EXPECT_STREQ ("1970-01-01T00:00:00-0000", buffer_.data ());
}

TEST_F (BasicLoggerTimeFixture, EpochInJST) {
    time_zone_setter tzs ("JST-9"); // Japan
    std::size_t const r =
        ::pstore::logging::basic_logger::time_string (std::time_t{0}, ::pstore::gsl2::make_span (buffer_));
    EXPECT_EQ (std::size_t{24}, r);
    EXPECT_EQ ('\0', buffer_[24]);
    EXPECT_STREQ ("1970-01-01T09:00:00+0900", buffer_.data ());
}

TEST_F (BasicLoggerTimeFixture, EpochInPST) {
    // Pacific Standard Time is 8 hours earlier than Coordinated Universal Time (UTC).
    // Standard time and daylight saving time both apply to this locale.
    // By default, Pacific Daylight Time is one hour ahead of standard time (that is, PDT7).
    // Since it isn't specified, daylight saving time starts on the first Sunday of April at 2:00
    // A.M., and ends on the last Sunday of October at 2:00 A.M.
    time_zone_setter tzs ("PST8PDT");
    std::size_t r =
        ::pstore::logging::basic_logger::time_string (std::time_t{0}, ::pstore::gsl2::make_span (buffer_));
    EXPECT_EQ (std::size_t{24}, r);
    EXPECT_EQ ('\0', buffer_[24]);
    EXPECT_STREQ ("1969-12-31T16:00:00-0800", buffer_.data ());
}

TEST_F (BasicLoggerTimeFixture, ArbitraryPointInTime) {
    time_zone_setter tzs ("UTC0");
    std::time_t const time{1447134860};
    std::size_t const r =
        ::pstore::logging::basic_logger::time_string (time, ::pstore::gsl2::make_span (buffer_));
    EXPECT_EQ (std::size_t{24}, r);
    this->canonicalize_sign ();
    EXPECT_STREQ ("2015-11-10T05:54:20-0000", buffer_.data ());
}


namespace {
    //***************************************************************
    //*   B a s i c L o g g e r T h r e a d N a m e F i x t u r e   *
    //***************************************************************
    class BasicLoggerThreadNameFixture : public ::testing::Test {
    public:
        BasicLoggerThreadNameFixture ();
        ~BasicLoggerThreadNameFixture ();

    private:
        std::string old_name_;
    };

    // (ctor)
    // ~~~~~~
    BasicLoggerThreadNameFixture::BasicLoggerThreadNameFixture ()
            : old_name_ (::pstore::threads::get_name ()) {}

    // (dtor)
    // ~~~~~~
    BasicLoggerThreadNameFixture::~BasicLoggerThreadNameFixture () {
        ::pstore::threads::set_name (old_name_.c_str ());
    }
} // end anonymous namespace

TEST_F (BasicLoggerThreadNameFixture, ThreadNameSet) {
    using ::testing::StrEq;
    ::pstore::threads::set_name ("mythreadname");
    EXPECT_THAT (::pstore::logging::basic_logger::get_current_thread_name ().c_str (),
                 StrEq ("mythreadname"));
}

TEST_F (BasicLoggerThreadNameFixture, ThreadNameEmpty) {
    using ::testing::AllOf;
    using ::testing::Ge;
    using ::testing::Le;

    ::pstore::threads::set_name ("");

    std::string const name = ::pstore::logging::basic_logger::get_current_thread_name ();

    // Here we're logging for '\([0-9]+\)'. Sadly regular expression support is extremely
    // limited on Windows, so it has to be done the hard way.
    std::size_t const length = name.length ();
    ASSERT_THAT (length, Ge (std::size_t{3}));
    EXPECT_EQ ('(', name[0]);
    for (unsigned index = 1; index < length - 2; ++index) {
        EXPECT_THAT (name[index], AllOf (Ge ('0'), Le ('9'))) << "Character at index " << index
                                                              << " was not a digit";
    }
    EXPECT_EQ (')', name[length - 1]);
}
// eof: unittests/pstore/test_basic_logger.cpp
