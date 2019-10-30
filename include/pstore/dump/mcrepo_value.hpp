//*                                                   _             *
//*  _ __ ___   ___ _ __ ___ _ __   ___   __   ____ _| |_   _  ___  *
//* | '_ ` _ \ / __| '__/ _ \ '_ \ / _ \  \ \ / / _` | | | | |/ _ \ *
//* | | | | | | (__| | |  __/ |_) | (_) |  \ V / (_| | | |_| |  __/ *
//* |_| |_| |_|\___|_|  \___| .__/ \___/    \_/ \__,_|_|\__,_|\___| *
//*                         |_|                                     *
//===- include/pstore/dump/mcrepo_value.hpp -------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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

#include "pstore/core/hamt_map.hpp"
#include "pstore/dump/value.hpp"
#include "pstore/mcrepo/compilation.hpp"
#include "pstore/mcrepo/fragment.hpp"

namespace pstore {
    namespace dump {

        value_ptr make_value (pstore::repo::section_kind t);
        value_ptr make_value (repo::internal_fixup const & ifx);
        value_ptr make_value (database const & db, repo::external_fixup const & xfx);

        value_ptr make_section_value (database const & db, repo::generic_section const & section,
                                      repo::section_kind sk, gsl::czstring triple, bool hex_mode);
        value_ptr make_section_value (database const & db, repo::dependents const & dependent,
                                      repo::section_kind sk, gsl::czstring triple, bool hex_mode);
        value_ptr make_section_value (database const & db, repo::debug_line_section const & section,
                                      repo::section_kind sk, gsl::czstring triple, bool hex_mode);
        value_ptr make_section_value (database const & db, repo::bss_section const & section,
                                      repo::section_kind sk, gsl::czstring triple, bool hex_mode);

        value_ptr make_fragment_value (database const & db, repo::fragment const & fragment,
                                       gsl::czstring triple, bool hex_mode);

        value_ptr make_value (repo::linkage l);
        value_ptr make_value (repo::visibility v);
        value_ptr make_value (database const & db, repo::compilation_member const & member);
        value_ptr make_value (database const & db,
                              std::shared_ptr<repo::compilation const> const & compilation);

        value_ptr make_value (database const & db, index::fragment_index::value_type const & value,
                              gsl::czstring triple, bool hex_mode);
        value_ptr make_value (database const & db,
                              index::compilation_index::value_type const & value);
        value_ptr make_value (database const & db,
                              index::debug_line_header_index::value_type const & value,
                              bool hex_mode);

    } // end namespace dump
} // end namespace pstore

#endif // PSTORE_DUMP_MCREPO_VALUE_HPP
