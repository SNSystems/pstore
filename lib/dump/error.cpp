//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===- lib/dump/error.cpp -------------------------------------------------===//
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
#include "pstore/dump/error.hpp"

// ******************
// * error category *
// ******************
// name
// ~~~~
char const * pstore::dump::error_category::name () const noexcept {
    return "pstore dump category";
}

// message
// ~~~~~~~
std::string pstore::dump::error_category::message (int error) const {
    static_assert (std::is_same<std::underlying_type<error_code>::type, decltype (error)>::value,
                   "base type of pstore::error_code must be int to permit safe static cast");
    switch (static_cast<error_code> (error)) {
    case error_code::cant_find_target: return "can't find target";
    case error_code::no_register_info_for_target: return "no register info for target";
    case error_code::no_assembly_info_for_target: return "no assembly info for target";
    case error_code::no_subtarget_info_for_target: return "no subtarget info_for_target";
    case error_code::no_instruction_info_for_target: return "no instruction info for target";
    case error_code::no_disassembler_for_target: return "no disassembler for target";
    }
    return "unknown value error";
}

namespace {

    // **********************
    // * get_error_category *
    // **********************
    std::error_category const & get_error_category () {
        static ::pstore::dump::error_category const cat;
        return cat;
    }

} // anonymous namespace

std::error_code std::make_error_code (pstore::dump::error_code e) {
    static_assert (std::is_same<std::underlying_type<decltype (e)>::type, int>::value,
                   "base type of error_code must be int to permit safe static cast");
    return {static_cast<int> (e), get_error_category ()};
}
