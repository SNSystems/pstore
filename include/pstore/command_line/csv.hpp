//===- include/pstore/command_line/csv.hpp ----------------*- mode: C++ -*-===//
//*                  *
//*   ___ _____   __ *
//*  / __/ __\ \ / / *
//* | (__\__ \\ V /  *
//*  \___|___/ \_/   *
//*                  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_CSV_HPP
#define PSTORE_COMMAND_LINE_CSV_HPP

#include <algorithm>
#include <list>
#include <string>

namespace pstore {
    namespace command_line {

        std::list<std::string> csv (std::string const & s);

        template <typename Iterator>
        std::list<std::string> csv (Iterator first, Iterator last) {
            std::list<std::string> result;
            std::for_each (first, last, [&result] (std::string const & s) {
                result.splice (result.end (), csv (s));
            });
            return result;
        }

    } // end namespace command_line
} // end namespace pstore

#endif // PSTORE_COMMAND_LINE_CSV_HPP
