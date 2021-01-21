//*                                                   _             *
//*  _ __ ___   ___ _ __ ___ _ __   ___   __   ____ _| |_   _  ___  *
//* | '_ ` _ \ / __| '__/ _ \ '_ \ / _ \  \ \ / / _` | | | | |/ _ \ *
//* | | | | | | (__| | |  __/ |_) | (_) |  \ V / (_| | | |_| |  __/ *
//* |_| |_| |_|\___|_|  \___| .__/ \___/    \_/ \__,_|_|\__,_|\___| *
//*                         |_|                                     *
//===- include/pstore/dump/mcrepo_value.hpp -------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#ifndef PSTORE_DUMP_MCREPO_VALUE_HPP
#define PSTORE_DUMP_MCREPO_VALUE_HPP

#include "pstore/core/hamt_map.hpp"
#include "pstore/dump/value.hpp"
#include "pstore/mcrepo/fragment.hpp"

namespace pstore {
    namespace dump {

        value_ptr make_value (pstore::repo::section_kind t);
        value_ptr make_value (repo::internal_fixup const & ifx);
        value_ptr make_value (database const & db, repo::external_fixup const & xfx);

        value_ptr make_section_value (database const & db, repo::generic_section const & section,
                                      repo::section_kind sk, gsl::czstring triple, bool hex_mode);
        value_ptr make_section_value (database const & db,
                                      repo::linked_definitions const & linked_definitions,
                                      repo::section_kind sk, gsl::czstring triple, bool hex_mode);
        value_ptr make_section_value (database const & db, repo::debug_line_section const & section,
                                      repo::section_kind sk, gsl::czstring triple, bool hex_mode);
        value_ptr make_section_value (database const & db, repo::bss_section const & section,
                                      repo::section_kind sk, gsl::czstring triple, bool hex_mode);

        value_ptr make_fragment_value (database const & db, repo::fragment const & fragment,
                                       gsl::czstring triple, bool hex_mode);

        value_ptr make_value (repo::linkage l);
        value_ptr make_value (repo::visibility v);
        value_ptr make_value (database const & db, repo::definition const & member);
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
