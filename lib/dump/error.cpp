//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===- lib/dump/error.cpp -------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
std::string pstore::dump::error_category::message (int const error) const {
    static_assert (std::is_same<std::underlying_type<error_code>::type, int>::value,
                   "base type of pstore::error_code must be int to permit safe static cast");

    auto * result = "unknown value error";
    switch (static_cast<error_code> (error)) {
    case error_code::cant_find_target: result = "can't find target"; break;
    case error_code::no_register_info_for_target: result = "no register info for target"; break;
    case error_code::no_assembly_info_for_target: result = "no assembly info for target"; break;
    case error_code::no_subtarget_info_for_target: result = "no subtarget info_for_target"; break;
    case error_code::no_instruction_info_for_target:
        result = "no instruction info for target";
        break;
    case error_code::no_disassembler_for_target: result = "no disassembler for target"; break;
    case error_code::no_instruction_printer: result = "no instruction printer for target"; break;
    case error_code::cannot_create_asm_streamer: result = "cannot create asm streamer"; break;
    case error_code::unknown_target: result = "unknown target"; break;
    }
    return result;
}

std::error_code pstore::dump::make_error_code (pstore::dump::error_code const e) {
    static_assert (std::is_same<std::underlying_type<decltype (e)>::type, int>::value,
                   "base type of error_code must be int to permit safe static cast");
    static error_category const cat;
    return {static_cast<int> (e), cat};
}
