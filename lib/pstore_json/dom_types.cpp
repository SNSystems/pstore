//*      _                   _                          *
//*   __| | ___  _ __ ___   | |_ _   _ _ __   ___  ___  *
//*  / _` |/ _ \| '_ ` _ \  | __| | | | '_ \ / _ \/ __| *
//* | (_| | (_) | | | | | | | |_| |_| | |_) |  __/\__ \ *
//*  \__,_|\___/|_| |_| |_|  \__|\__, | .__/ \___||___/ *
//*                              |___/|_|               *
//===- lib/pstore_json/dom_types.cpp --------------------------------------===//
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

#include "pstore_json/dom_types.hpp"
#include <algorithm>

namespace pstore {
    namespace json {
        namespace value {

            dom_element::~dom_element () noexcept = default;
            null_value::~null_value () noexcept = default;
            number_base::~number_base () noexcept = default;
            number_double::~number_double () noexcept = default;
            number_long::~number_long () noexcept = default;
            string_value::~string_value () noexcept = default;

            std::ostream & boolean_value::write (std::ostream & os) const {
                return os << (b_ ? "true" : "false");
            }
            std::ostream & array_value::write (std::ostream & os) const {
                os << '[';
                auto separator = "";
                for (auto const & v : v_) {
                    os << separator;
                    v->write (os);
                    separator = ", ";
                }
                return os << ']';
            }
            std::ostream & object_value::write (std::ostream & os) const {
                os << '{';
                char const * separator = "";
                for (auto const & v : v_) {
                    os << separator << '"' << v.first << "\": ";
                    v.second->write (os);
                    separator = ", ";
                }
                return os << '}';
            }

        } // namespace value


        void yaml_output::begin_array () { out_.emplace (nullptr); }
        void yaml_output::end_array () {
            value::array_value::container_type content;
            for (;;) {
                auto v = out_.top ();
                out_.pop ();
                if (!v) {
                    break;
                }
                content.push_back (std::move (v));
            }
            std::reverse (std::begin (content), std::end (content));
            out_.emplace (new value::array_value (content));
        }

        void yaml_output::begin_object () { out_.emplace (nullptr); }
        void yaml_output::end_object () {
            auto object = std::make_shared<value::object_value> ();
            for (;;) {
                auto value = out_.top ();
                out_.pop ();
                if (!value) {
                    break;
                }

                auto key = out_.top ();
                out_.pop ();
                assert (key);
                auto key_str = key->as_string ();
                assert (key_str);
                object->insert (key_str->get (), value);
            }
            out_.emplace (std::move (object));
        }
    } // namespace json
} // namespace pstore
// eof: lib/pstore_json/dom_types.cpp
