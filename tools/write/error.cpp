//===- tools/write/error.cpp ----------------------------------------------===//
//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "error.hpp"

char const * write_error_category::name () const noexcept {
    return "pstore_write category";
}

std::string write_error_category::message (int error) const {
    auto * result = "unknown error";
    switch (static_cast<write_error_code> (error)) {
    case write_error_code::unrecognized_compaction_mode:
        result = "unrecognized compaction mode";
        break;
    }
    return result;
}

std::error_code make_error_code (write_error_code e) {
    static_assert (std::is_same<std::underlying_type<decltype (e)>::type, int>::value,
                   "base type of error_code must be int to permit safe static cast");

    static write_error_category const cat;
    return {static_cast<int> (e), cat};
}
