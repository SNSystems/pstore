//*      _                   _                          *
//*   __| | ___  _ __ ___   | |_ _   _ _ __   ___  ___  *
//*  / _` |/ _ \| '_ ` _ \  | __| | | | '_ \ / _ \/ __| *
//* | (_| | (_) | | | | | | | |_| |_| | |_) |  __/\__ \ *
//*  \__,_|\___/|_| |_| |_|  \__|\__, | .__/ \___||___/ *
//*                              |___/|_|               *
//===- include/pstore_json/dom_types.hpp ----------------------------------===//
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
/// \file dom_types.cpp

#ifndef PSTORE_JSON_DOM_TYPES_HPP
#define PSTORE_JSON_DOM_TYPES_HPP

#include <cassert>
#include <string>
#include <unordered_map>
#include <memory>
#include <ostream>
#include <stack>
#include <vector>

#include "dump/value.hpp"

namespace pstore {
    namespace json {

        // FIXME: call this "dom" or somesuch.
        class yaml_output {
        public:
            using result_type = std::shared_ptr<dump::value>;

            void string_value (std::string const & s) { out_.emplace (new class dump::string (s)); }
            void integer_value (long v) { out_.emplace (dump::make_number (v)); }
            void float_value (double v) { out_.emplace (dump::make_number (v)); }
            void boolean_value (bool v) { out_.emplace (new dump::boolean (v)); }
            void null_value () { out_.emplace (new dump::null ()); }

            void begin_array ();
            void end_array ();

            void begin_object ();
            void end_object ();

            result_type result () const {
                assert (out_.size () == 1U);
                return out_.top ();
            }

        private:
            std::stack<result_type> out_;
        };
    } // namespace json
} // namespace pstore

#endif // PSTORE_JSON_DOM_TYPES_HPP
// eof: include/pstore_json/dom_types.hpp
