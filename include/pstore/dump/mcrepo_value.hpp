//*                                                   _             *
//*  _ __ ___   ___ _ __ ___ _ __   ___   __   ____ _| |_   _  ___  *
//* | '_ ` _ \ / __| '__/ _ \ '_ \ / _ \  \ \ / / _` | | | | |/ _ \ *
//* | | | | | | (__| | |  __/ |_) | (_) |  \ V / (_| | | |_| |  __/ *
//* |_| |_| |_|\___|_|  \___| .__/ \___/    \_/ \__,_|_|\__,_|\___| *
//*                         |_|                                     *
//===- include/pstore/dump/mcrepo_value.hpp -------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_DUMP_MCREPO_VALUE_HPP
#define PSTORE_DUMP_MCREPO_VALUE_HPP

#include "pstore/dump/value.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/mcrepo/ticket.hpp"

namespace pstore {
    namespace dump {

        value_ptr make_value (repo::fragment_type t);
        value_ptr make_value (repo::internal_fixup const & ifx);
        value_ptr make_value (database & db, repo::external_fixup const & xfx);
        value_ptr make_value (database & db, repo::section const & section, repo::fragment_type st,
                              bool hex_mode);
        value_ptr make_value (database & db, repo::dependents const & dependent,
                              repo::fragment_type st, bool hex_mode);
        value_ptr make_value (database & db, repo::fragment const & fragment, bool hex_mode);


        value_ptr make_fragments (database & db, bool hex_mode);

        value_ptr make_value (repo::linkage_type t);
        value_ptr make_value (database const & db, repo::ticket_member const & member);
        value_ptr make_value (database const & db, std::shared_ptr<repo::ticket const> ticket);

        value_ptr make_tickets (database & db);

    } // namespace dump
} // namespace pstore

#endif // PSTORE_DUMP_MCREPO_VALUE_HPP
