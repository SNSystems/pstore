//===- lib/command_line/modifiers.cpp -------------------------------------===//
//*                      _ _  __ _                *
//*  _ __ ___   ___   __| (_)/ _(_) ___ _ __ ___  *
//* | '_ ` _ \ / _ \ / _` | | |_| |/ _ \ '__/ __| *
//* | | | | | | (_) | (_| | |  _| |  __/ |  \__ \ *
//* |_| |_| |_|\___/ \__,_|_|_| |_|\___|_|  |___/ *
//*                                               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/modifiers.hpp"

#include <algorithm>

namespace pstore {
    namespace command_line {

            details::values::values (std::initializer_list<literal> options)
                    : values_{std::move (options)} {}

            name::name (std::string name)
                    : name_{std::move (name)} {}

            usage::usage (std::string str)
                    : desc_{std::move (str)} {}

            desc::desc (std::string str)
                    : desc_{std::move (str)} {}

            aliasopt::aliasopt (option & o)
                    : original_ (o) {}
            void aliasopt::apply (alias & o) const { o.set_original (&original_); }

            details::comma_separated const comma_separated;

            details::one_or_more const one_or_more;
            details::optional const optional;
            details::positional const positional;
            details::required const required;

    } // end namespace command_line
} // end namespace pstore
