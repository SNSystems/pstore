//*            _ _ _                _         *
//*   ___ __ _| | | |__   __ _  ___| | _____  *
//*  / __/ _` | | | '_ \ / _` |/ __| |/ / __| *
//* | (_| (_| | | | |_) | (_| | (__|   <\__ \ *
//*  \___\__,_|_|_|_.__/ \__,_|\___|_|\_\___/ *
//*                                           *
//===- unittests/json/callbacks.hpp ---------------------------------------===//
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

#ifndef PSTORE_UNIT_TESTS_JSON_CALLBACKS_HPP
#define PSTORE_UNIT_TESTS_JSON_CALLBACKS_HPP

#include <system_error>
#include <gmock/gmock.h>

class json_callbacks_base {
public:
    virtual ~json_callbacks_base ();

    virtual std::error_code string_value (std::string const &) = 0;
    virtual std::error_code int64_value (std::int64_t) = 0;
    virtual std::error_code uint64_value (std::uint64_t) = 0;
    virtual std::error_code double_value (double) = 0;
    virtual std::error_code boolean_value (bool) = 0;
    virtual std::error_code null_value () = 0;
    virtual std::error_code begin_array () = 0;
    virtual std::error_code end_array () = 0;
    virtual std::error_code begin_object () = 0;
    virtual std::error_code key (std::string const &) = 0;
    virtual std::error_code end_object () = 0;
};

class mock_json_callbacks : public json_callbacks_base {
public:
    ~mock_json_callbacks ();

    MOCK_METHOD1 (string_value, std::error_code (std::string const &));
    MOCK_METHOD1 (int64_value, std::error_code (std::int64_t));
    MOCK_METHOD1 (uint64_value, std::error_code (std::uint64_t));
    MOCK_METHOD1 (double_value, std::error_code (double));
    MOCK_METHOD1 (boolean_value, std::error_code (bool));
    MOCK_METHOD0 (null_value, std::error_code ());
    MOCK_METHOD0 (begin_array, std::error_code ());
    MOCK_METHOD0 (end_array, std::error_code ());
    MOCK_METHOD0 (begin_object, std::error_code ());
    MOCK_METHOD1 (key, std::error_code (std::string const &));
    MOCK_METHOD0 (end_object, std::error_code ());
};

template <typename T>
class callbacks_proxy {
public:
    using result_type = void;
    result_type result () {}

    explicit callbacks_proxy (T & original)
            : original_ (original) {}
    callbacks_proxy (callbacks_proxy const &) = default;

    std::error_code string_value (std::string const & s) { return original_.string_value (s); }
    std::error_code int64_value (std::int64_t v) { return original_.int64_value (v); }
    std::error_code uint64_value (std::uint64_t v) { return original_.uint64_value (v); }
    std::error_code double_value (double v) { return original_.double_value (v); }
    std::error_code boolean_value (bool v) { return original_.boolean_value (v); }
    std::error_code null_value () { return original_.null_value (); }
    std::error_code begin_array () { return original_.begin_array (); }
    std::error_code end_array () { return original_.end_array (); }
    std::error_code begin_object () { return original_.begin_object (); }
    std::error_code key (std::string const & s) { return original_.key (s); }
    std::error_code end_object () { return original_.end_object (); }

private:
    T & original_;
};

#endif // PSTORE_UNIT_TESTS_JSON_CALLBACKS_H
