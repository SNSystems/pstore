//*  _                            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| *
//* | | | | | | | |_) | (_) | |  | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| *
//*             |_|                    *
//*   __                                      _    *
//*  / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                |___/                           *
//===- tools/import/import_fragment.hpp -----------------------------------===//
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
#ifndef PSTORE_IMPORT_IMPORT_FRAGMENT_HPP
#define PSTORE_IMPORT_IMPORT_FRAGMENT_HPP

#include <bitset>
#include <vector>

#include "pstore/core/database.hpp"
#include "pstore/mcrepo/generic_section.hpp"

#include "import_rule.hpp"

class generic_section final : public state {
public:
    generic_section (parse_stack_pointer stack);

private:
    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;
    pstore::gsl::czstring name () const noexcept override;

    enum { data, align, ifixups, xfixups };
    std::bitset<xfixups + 1> seen_;

    std::string data_;
    std::uint64_t align_ = 0;
    std::vector<pstore::repo::internal_fixup> ifixups_;
    std::vector<pstore::repo::external_fixup> xfixups_;
};

class debug_line_section final : public state {
public:
    debug_line_section (parse_stack_pointer stack);

private:
    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;
    pstore::gsl::czstring name () const noexcept override;

    enum { header, data, ifixups };
    std::bitset<ifixups + 1> seen_;

    std::string header_;
    std::string data_;
    std::vector<pstore::repo::internal_fixup> ifixups_;
};

#endif // PSTORE_IMPORT_IMPORT_FRAGMENT_HPP
