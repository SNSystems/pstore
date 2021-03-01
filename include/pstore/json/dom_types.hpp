//===- include/pstore/json/dom_types.hpp ------------------*- mode: C++ -*-===//
//*      _                   _                          *
//*   __| | ___  _ __ ___   | |_ _   _ _ __   ___  ___  *
//*  / _` |/ _ \| '_ ` _ \  | __| | | | '_ \ / _ \/ __| *
//* | (_| | (_) | | | | | | | |_| |_| | |_) |  __/\__ \ *
//*  \__,_|\___/|_| |_| |_|  \__|\__, | .__/ \___||___/ *
//*                              |___/|_|               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef PSTORE_JSON_DOM_TYPES_HPP
#define PSTORE_JSON_DOM_TYPES_HPP

#include <cstdint>
#include <string>
#include <system_error>

namespace pstore {
    namespace json {

        class null_output {
        public:
            using result_type = void;

            std::error_code string_value (std::string const &) const noexcept { return {}; }
            std::error_code int64_value (std::int64_t const) const noexcept { return {}; }
            std::error_code uint64_value (std::uint64_t const) const noexcept { return {}; }
            std::error_code double_value (double const) const noexcept { return {}; }
            std::error_code boolean_value (bool const) const noexcept { return {}; }
            std::error_code null_value () const noexcept { return {}; }

            std::error_code begin_array () const noexcept { return {}; }
            std::error_code end_array () const noexcept { return {}; }

            std::error_code begin_object () const noexcept { return {}; }
            std::error_code key (std::string const &) const noexcept { return {}; }
            std::error_code end_object () const noexcept { return {}; }

            result_type result () const noexcept {}
        };

    } // end namespace json
} // end namespace pstore

#endif // PSTORE_JSON_DOM_TYPES_HPP
