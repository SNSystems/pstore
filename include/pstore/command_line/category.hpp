//*            _                               *
//*   ___ __ _| |_ ___  __ _  ___  _ __ _   _  *
//*  / __/ _` | __/ _ \/ _` |/ _ \| '__| | | | *
//* | (_| (_| | ||  __/ (_| | (_) | |  | |_| | *
//*  \___\__,_|\__\___|\__, |\___/|_|   \__, | *
//*                    |___/            |___/  *
//===- include/pstore/command_line/category.hpp ---------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file category.hpp
/// \brief Defines option_category; a means to group switches in command-line help text.

#ifndef PSTORE_COMMAND_LINE_CATEGORY_HPP
#define PSTORE_COMMAND_LINE_CATEGORY_HPP

#include <string>

namespace pstore {
    namespace command_line {

        class option_category {
        public:
            explicit option_category (std::string const & title);
            std::string const & title () const noexcept { return title_; }

        private:
            std::string title_;
        };

    } // end namespace command_line
} // end namespace pstore

#endif // PSTORE_COMMAND_LINE_CATEGORY_HPP
