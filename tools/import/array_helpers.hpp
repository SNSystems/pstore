//*                               _          _                      *
//*   __ _ _ __ _ __ __ _ _   _  | |__   ___| |_ __   ___ _ __ ___  *
//*  / _` | '__| '__/ _` | | | | | '_ \ / _ \ | '_ \ / _ \ '__/ __| *
//* | (_| | |  | | | (_| | |_| | | | | |  __/ | |_) |  __/ |  \__ \ *
//*  \__,_|_|  |_|  \__,_|\__, | |_| |_|\___|_| .__/ \___|_|  |___/ *
//*                       |___/               |_|                   *
//===- tools/import/array_helpers.hpp -------------------------------------===//
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
#ifndef PSTORE_IMPORT_ARRAY_HELPERS_HPP
#define PSTORE_IMPORT_ARRAY_HELPERS_HPP

#include "import_rule.hpp"

template <typename T, typename Next>
class array_object final : public state {
public:
    array_object (parse_stack_pointer s, pstore::gsl::not_null<std::vector<T> *> arr)
            : state (s)
            , arr_{arr} {}
    std::error_code begin_object () override { return push<Next> (arr_); }
    std::error_code end_array () override { return pop (); }

private:
    pstore::gsl::not_null<std::vector<T> *> arr_;
};



#endif // PSTORE_IMPORT_ARRAY_HELPERS_HPP
