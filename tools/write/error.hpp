//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===- tools/write/error.hpp ----------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#ifndef PSTORE_WRITE_ERROR_HPP
#define PSTORE_WRITE_ERROR_HPP

#include <string>
#include <system_error>
#include <type_traits>

// ********************
// * write error code *
// ********************
enum class write_error_code : int { unrecognized_compaction_mode = 1 };


// ************************
// * write error category *
// ************************
class write_error_category : public std::error_category {
public:
    // The need for this constructor was removed by CWG defect 253 but Clang (prior to 3.9.0) and
    // GCC (before 4.6.4) require its presence.
    write_error_category () noexcept {}
    char const * name () const noexcept override;
    std::string message (int error) const override;
};

std::error_code make_error_code (write_error_code e);

namespace std {

    template <>
    struct is_error_code_enum<write_error_category> : std::true_type {};

} // namespace std

#endif // PSTORE_WRITE_ERROR_HPP
