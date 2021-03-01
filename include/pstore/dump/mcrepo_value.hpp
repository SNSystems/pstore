//===- include/pstore/dump/mcrepo_value.hpp ---------------*- mode: C++ -*-===//
//*                                                   _             *
//*  _ __ ___   ___ _ __ ___ _ __   ___   __   ____ _| |_   _  ___  *
//* | '_ ` _ \ / __| '__/ _ \ '_ \ / _ \  \ \ / / _` | | | | |/ _ \ *
//* | | | | | | (__| | |  __/ |_) | (_) |  \ V / (_| | | |_| |  __/ *
//* |_| |_| |_|\___|_|  \___| .__/ \___/    \_/ \__,_|_|\__,_|\___| *
//*                         |_|                                     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_DUMP_MCREPO_VALUE_HPP
#define PSTORE_DUMP_MCREPO_VALUE_HPP

#include "pstore/core/hamt_map.hpp"
#include "pstore/dump/parameter.hpp"
#include "pstore/dump/value.hpp"
#include "pstore/mcrepo/fragment.hpp"

namespace pstore {
    namespace dump {

        value_ptr make_value (repo::section_kind t);
        value_ptr make_value (repo::internal_fixup const & ifx, parameters const & parm);
        value_ptr make_value (repo::external_fixup const & xfx, parameters const & parm);

        value_ptr make_section_value (repo::generic_section const & section, repo::section_kind sk,
                                      parameters const & parm);
        value_ptr make_section_value (repo::linked_definitions const & linked_definitions,
                                      repo::section_kind sk, parameters const & parm);
        value_ptr make_section_value (repo::debug_line_section const & section,
                                      repo::section_kind sk, parameters const & parm);
        value_ptr make_section_value (repo::bss_section const & section, repo::section_kind sk,
                                      parameters const & s);

        value_ptr make_fragment_value (repo::fragment const & fragment, parameters const & parm);

        value_ptr make_value (repo::linkage l);
        value_ptr make_value (repo::visibility v);
        value_ptr make_value (repo::definition const & member, parameters const & parm);
        value_ptr make_value (std::shared_ptr<repo::compilation const> const & compilation,
                              parameters const & parm);

        value_ptr make_value (index::fragment_index::value_type const & value,
                              parameters const & parm);
        value_ptr make_value (index::compilation_index::value_type const & value,
                              parameters const & parm);
        value_ptr make_value (index::debug_line_header_index::value_type const & value,
                              parameters const & parm);

    } // end namespace dump
} // end namespace pstore

#endif // PSTORE_DUMP_MCREPO_VALUE_HPP
