//===- unittests/json/callbacks.hpp -----------------------*- mode: C++ -*-===//
//*            _ _ _                _         *
//*   ___ __ _| | | |__   __ _  ___| | _____  *
//*  / __/ _` | | | '_ \ / _` |/ __| |/ / __| *
//* | (_| (_| | | | |_) | (_| | (__|   <\__ \ *
//*  \___\__,_|_|_|_.__/ \__,_|\___|_|\_\___/ *
//*                                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
    ~mock_json_callbacks () override;

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

class json_out_callbacks {
public:
    using result_type = std::string;
    result_type result () { return out_; }

    std::error_code string_value (std::string const & s) {
        append ('"' + s + '"');
        return {};
    }
    std::error_code int64_value (std::int64_t v) {
        append (std::to_string (v));
        return {};
    }
    std::error_code uint64_value (std::uint64_t v) {
        append (std::to_string (v));
        return {};
    }
    std::error_code double_value (double v) {
        append (std::to_string (v));
        return {};
    }
    std::error_code boolean_value (bool v) {
        append (v ? "true" : "false");
        return {};
    }
    std::error_code null_value () {
        append ("null");
        return {};
    }

    std::error_code begin_array () {
        append ("[");
        return {};
    }
    std::error_code end_array () {
        append ("]");
        return {};
    }

    std::error_code begin_object () {
        append ("{");
        return {};
    }
    std::error_code key (std::string const & s) {
        string_value (s);
        return {};
    }
    std::error_code end_object () {
        append ("}");
        return {};
    }

private:
    template <typename StringType>
    void append (StringType && s) {
        if (out_.length () > 0) {
            out_ += ' ';
        }
        out_ += s;
    }

    std::string out_;
};

#endif // PSTORE_UNIT_TESTS_JSON_CALLBACKS_H
