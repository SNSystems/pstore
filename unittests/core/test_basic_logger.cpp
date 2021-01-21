//*  _               _        _                              *
//* | |__   __ _ ___(_) ___  | | ___   __ _  __ _  ___ _ __  *
//* | '_ \ / _` / __| |/ __| | |/ _ \ / _` |/ _` |/ _ \ '__| *
//* | |_) | (_| \__ \ | (__  | | (_) | (_| | (_| |  __/ |    *
//* |_.__/ \__,_|___/_|\___| |_|\___/ \__, |\__, |\___|_|    *
//*                                   |___/ |___/            *
//===- unittests/core/test_basic_logger.cpp -------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file test_basic_logger.cpp

#include "pstore/os/logging.hpp"

// standard library
#include <cstring>

// pstore includes
#include "pstore/os/thread.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/maybe.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/utf.hpp"

// 3rd party
#include <gmock/gmock.h>

using pstore::just;
using pstore::maybe;
using pstore::nothing;
using pstore::gsl::czstring;
using pstore::gsl::not_null;

namespace {

    //***************************************
    //*   t i m e _ z o n e _ s e t t e r   *
    // **************************************
    class time_zone_setter {
    public:
        explicit time_zone_setter (czstring tz);
        ~time_zone_setter () noexcept;

    private:
        static maybe<std::string> tz_value ();

        static void set_tz (not_null<czstring> tz);
        static void setenv (not_null<czstring> name, not_null<czstring> value);
        static void unsetenv (not_null<czstring> name);

        maybe<std::string> old_;
    };

    // (ctor)
    // ~~~~~~
    time_zone_setter::time_zone_setter (czstring tz)
            : old_ (time_zone_setter::tz_value ()) {
        time_zone_setter::set_tz (tz);
    }

    // (dtor)
    // ~~~~~~
    time_zone_setter::~time_zone_setter () noexcept {
        pstore::no_ex_escape ([this] {
            if (old_) {
                time_zone_setter::setenv ("TZ", old_->c_str ());
            } else {
                time_zone_setter::unsetenv ("TZ");
            }
        });
    }

    // tz_value
    // ~~~~~~~~
    maybe<std::string> time_zone_setter::tz_value () {
        czstring const tz = std::getenv ("TZ");
        return tz != nullptr ? just (std::string{tz}) : nothing<std::string> ();
    }

    // set
    // ~~~
    void time_zone_setter::set_tz (not_null<czstring> tz) { time_zone_setter::setenv ("TZ", tz); }

    // setenv
    // ~~~~~~
    void time_zone_setter::setenv (not_null<czstring> name, not_null<czstring> value) {
#ifdef _WIN32
        using pstore::utf::win32::to16;
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

    // unsetenv
    // ~~~~~~~~
    void time_zone_setter::unsetenv (not_null<czstring> name) {
#ifdef _WIN32
        using pstore::utf::win32::to16;
        ::_wputenv_s (to16 (name).c_str (), L"");
#else
        ::unsetenv (name);
#endif
    }

    //***************************************************
    //*   B a s i c L o g g e r T i m e F i x t u r e   *
    //***************************************************
    class BasicLoggerTimeFixture : public testing::Test {
    public:
        std::array<char, pstore::basic_logger::time_buffer_size> buffer_;
        static constexpr unsigned const sign_index_ = 19;

        // If the time zone offset is 0, the standard library could legimately describe that
        // as either +0000 or -0000. Canonicalize here (to -0000).
        void canonicalize_sign ();
    };

    // canonicalize_sign
    // ~~~~~~~~~~~~~~~~~
    void BasicLoggerTimeFixture::canonicalize_sign () {
        static_assert (pstore::basic_logger::time_buffer_size >= sign_index_,
                       "sign index is too large for time_buffer");
        ASSERT_EQ ('\0', buffer_[pstore::basic_logger::time_buffer_size - 1]);
        if (std::strcmp (&buffer_[sign_index_], "+0000") == 0) {
            buffer_[sign_index_] = '-';
        }
    }

} // end anonymous namespace

TEST_F (BasicLoggerTimeFixture, EpochInUTC) {
    time_zone_setter tzs ("UTC0");
    std::size_t const r =
        pstore::basic_logger::time_string (std::time_t{0}, pstore::gsl::make_span (buffer_));
    EXPECT_EQ (std::size_t{24}, r);
    EXPECT_EQ ('\0', buffer_[24]);
    this->canonicalize_sign ();
    EXPECT_STREQ ("1970-01-01T00:00:00-0000", buffer_.data ());
}

TEST_F (BasicLoggerTimeFixture, EpochInJST) {
    time_zone_setter tzs ("JST-9"); // Japan
    std::size_t const r =
        pstore::basic_logger::time_string (std::time_t{0}, pstore::gsl::make_span (buffer_));
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
        pstore::basic_logger::time_string (std::time_t{0}, pstore::gsl::make_span (buffer_));
    EXPECT_EQ (std::size_t{24}, r);
    EXPECT_EQ ('\0', buffer_[24]);
    EXPECT_STREQ ("1969-12-31T16:00:00-0800", buffer_.data ());
}

TEST_F (BasicLoggerTimeFixture, ArbitraryPointInTime) {
    time_zone_setter tzs ("UTC0");
    std::time_t const time{1447134860};
    std::size_t const r =
        pstore::basic_logger::time_string (time, pstore::gsl::make_span (buffer_));
    EXPECT_EQ (std::size_t{24}, r);
    this->canonicalize_sign ();
    EXPECT_STREQ ("2015-11-10T05:54:20-0000", buffer_.data ());
}


namespace {

    //***************************************************************
    //*   B a s i c L o g g e r T h r e a d N a m e F i x t u r e   *
    //***************************************************************
    class BasicLoggerThreadNameFixture : public testing::Test {
    public:
        BasicLoggerThreadNameFixture ();
        ~BasicLoggerThreadNameFixture ();

    private:
        std::string old_name_;
    };

    // (ctor)
    // ~~~~~~
    BasicLoggerThreadNameFixture::BasicLoggerThreadNameFixture ()
            : old_name_ (pstore::threads::get_name ()) {}

    // (dtor)
    // ~~~~~~
    BasicLoggerThreadNameFixture::~BasicLoggerThreadNameFixture () {
        pstore::no_ex_escape ([this] () { pstore::threads::set_name (old_name_.c_str ()); });
    }

} // end anonymous namespace

TEST_F (BasicLoggerThreadNameFixture, ThreadNameSet) {
    using testing::StrEq;
    pstore::threads::set_name ("mythreadname");
    EXPECT_THAT (pstore::basic_logger::get_current_thread_name ().c_str (), StrEq ("mythreadname"));
}

TEST_F (BasicLoggerThreadNameFixture, ThreadNameEmpty) {
    using testing::AllOf;
    using testing::Ge;
    using testing::Le;

    pstore::threads::set_name ("");

    std::string const name = pstore::basic_logger::get_current_thread_name ();

    // Here we're looking for '\([0-9]+\)'. Sadly regular expression support is extremely
    // limited on Windows, so it has to be done the hard way.
    std::size_t const length = name.length ();
    ASSERT_THAT (length, Ge (std::size_t{3}));
    EXPECT_EQ ('(', name[0]);
    for (unsigned index = 1; index < length - 2; ++index) {
        EXPECT_THAT (name[index], AllOf (Ge ('0'), Le ('9')))
            << "Character at index " << index << " was not a digit";
    }
    EXPECT_EQ (')', name[length - 1]);
}
