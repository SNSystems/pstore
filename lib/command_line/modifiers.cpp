//*                      _ _  __ _                *
//*  _ __ ___   ___   __| (_)/ _(_) ___ _ __ ___  *
//* | '_ ` _ \ / _ \ / _` | | |_| |/ _ \ '__/ __| *
//* | | | | | | (_) | (_| | |  _| |  __/ |  \__ \ *
//* |_| |_| |_|\___/ \__,_|_|_| |_|\___|_|  |___/ *
//*                                               *
//===- lib/command_line/modifiers.cpp -------------------------------------===//
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
#include "pstore/command_line/modifiers.hpp"

#include <algorithm>

namespace pstore {
    namespace command_line {
        namespace cl {

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

        } // namespace cl
    }     // namespace command_line
} // namespace pstore
