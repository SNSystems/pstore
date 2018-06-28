//*            _ _ _                _         *
//*   ___ __ _| | | |__   __ _  ___| | _____  *
//*  / __/ _` | | | '_ \ / _` |/ __| |/ / __| *
//* | (_| (_| | | | |_) | (_| | (__|   <\__ \ *
//*  \___\__,_|_|_|_.__/ \__,_|\___|_|\_\___/ *
//*                                           *
//===- unittests/pstore_json/callbacks.hpp --------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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

#ifndef callbacks_h
#define callbacks_h

#include <gmock/gmock.h>

class json_callbacks_base {
public:
    virtual ~json_callbacks_base () {}

    virtual void string_value (std::string const &) = 0;
    virtual void integer_value (long) = 0;
    virtual void float_value (double) = 0;
    virtual void boolean_value (bool) = 0;
    virtual void null_value () = 0;
    virtual void begin_array () = 0;
    virtual void end_array () = 0;
    virtual void begin_object () = 0;
    virtual void end_object () = 0;
};

class mock_json_callbacks : public json_callbacks_base {
public:
    MOCK_METHOD1 (string_value, void(std::string const &));
    MOCK_METHOD1 (integer_value, void(long));
    MOCK_METHOD1 (float_value, void(double));
    MOCK_METHOD1 (boolean_value, void(bool));
    MOCK_METHOD0 (null_value, void());
    MOCK_METHOD0 (begin_array, void());
    MOCK_METHOD0 (end_array, void());
    MOCK_METHOD0 (begin_object, void());
    MOCK_METHOD0 (end_object, void());
};

template <typename T>
class callbacks_proxy {
public:
    using result_type = void;
    result_type result () {}

    explicit callbacks_proxy (T & original)
            : original_{original} {}
    callbacks_proxy (callbacks_proxy const &) = default;

    void string_value (std::string const & s) { original_.string_value (s); }
    void integer_value (long v) { original_.integer_value (v); }
    void float_value (double v) { original_.float_value (v); }
    void boolean_value (bool v) { original_.boolean_value (v); }
    void null_value () { original_.null_value (); }
    void begin_array () { original_.begin_array (); }
    void end_array () { original_.end_array (); }
    void begin_object () { original_.begin_object (); }
    void end_object () { original_.end_object (); }

private:
    T & original_;
};

#endif /* callbacks_h */
// eof: unittests/pstore_json/callbacks.hpp
