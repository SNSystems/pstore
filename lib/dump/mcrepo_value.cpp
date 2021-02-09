//*                                                   _             *
//*  _ __ ___   ___ _ __ ___ _ __   ___   __   ____ _| |_   _  ___  *
//* | '_ ` _ \ / __| '__/ _ \ '_ \ / _ \  \ \ / / _` | | | | |/ _ \ *
//* | | | | | | (__| | |  __/ |_) | (_) |  \ V / (_| | | |_| |  __/ *
//* |_| |_| |_|\___|_|  \___| .__/ \___/    \_/ \__,_|_|\__,_|\___| *
//*                         |_|                                     *
//===- lib/dump/mcrepo_value.cpp ------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/dump/mcrepo_value.hpp"

#include "pstore/config/config.hpp"
#include "pstore/dump/db_value.hpp"
#include "pstore/dump/mcdebugline_value.hpp"
#include "pstore/dump/mcdisassembler_value.hpp"
#include "pstore/dump/value.hpp"
#include "pstore/core/hamt_set.hpp"

namespace pstore {
    namespace dump {

        value_ptr make_value (repo::section_kind const t) {
            char const * name = "*unknown*";
#define X(k)                                                                                       \
    case repo::section_kind::k: name = #k; break;

            switch (t) {
                PSTORE_MCREPO_SECTION_KINDS
            case repo::section_kind::last: break;
            }
            return dump::make_value (name);
#undef X
        }

        value_ptr make_value (repo::internal_fixup const & ifx) {
            auto const v = std::make_shared<object> (object::container{
                {"section", make_value (ifx.section)},
                {"type", make_value (static_cast<std::uint64_t> (ifx.type))},
                {"offset", make_value (ifx.offset)},
                {"addend", make_value (ifx.addend)},
            });
            v->compact ();
            return std::static_pointer_cast<value> (v);
        }

        value_ptr make_value (database const & db, repo::external_fixup const & xfx) {
            using serialize::read;
            using serialize::archive::make_reader;

            return make_value (object::container{
                {"name", make_value (indirect_string::read (db, xfx.name))},
                {"type", make_value (static_cast<std::uint64_t> (xfx.type))},
                {"is_weak", make_value (xfx.is_weak)},
                {"offset", make_value (xfx.offset)},
                {"addend", make_value (xfx.addend)},
            });
        }

        value_ptr make_section_value (database const & db, repo::generic_section const & section,
                                      repo::section_kind const sk, gsl::czstring const triple,
                                      bool const hex_mode) {

            (void) sk;
            (void) triple;
            auto const & payload = section.payload ();
            value_ptr data_value;
#ifdef PSTORE_IS_INSIDE_LLVM
            if (sk == repo::section_kind::text) {
                std::uint8_t const * const first = payload.data ();
                data_value =
                    make_disassembled_value (first, first + payload.size (), triple, hex_mode);
            }
#endif // PSTORE_IS_INSIDE_LLVM
            if (!data_value) {
                if (hex_mode) {
                    data_value =
                        std::make_shared<binary16> (std::begin (payload), std::end (payload));
                } else {
                    data_value =
                        std::make_shared<binary> (std::begin (payload), std::end (payload));
                }
            }
            return make_value (object::container{
                {"align", make_value (section.align ())},
                {"data", data_value},
                {"ifixups",
                 make_value (std::begin (section.ifixups ()), std::end (section.ifixups ()))},
                {"xfixups",
                 make_value (
                     std::begin (section.xfixups ()), std::end (section.xfixups ()),
                     [&db] (repo::external_fixup const & xfx) { return make_value (db, xfx); })},
            });
        }

        value_ptr make_section_value (database const & db,
                                      repo::linked_definitions const & linked_definitions,
                                      repo::section_kind const /*sk*/,
                                      gsl::czstring const /*triple*/, bool const /*hex_mode*/) {
            return make_value (std::begin (linked_definitions), std::end (linked_definitions),
                               [&db] (repo::linked_definitions::value_type const & member) {
                                   return make_value (object::container{
                                       {"compilation", make_value (member.compilation)},
                                       {"index", make_value (member.index)},
                                       {"pointer", make_value (db, *db.getro (member.pointer))},
                                   });
                               });
        }

        value_ptr make_section_value (database const & db, repo::debug_line_section const & section,
                                      repo::section_kind const sk, gsl::czstring const triple,
                                      bool const hex_mode) {
            PSTORE_ASSERT (sk == repo::section_kind::debug_line);
            return make_value (object::container{
                {"digest", make_value (section.header_digest ())},
                {"header", make_value (section.header_extent ())},
                {"generic", make_section_value (db, section.generic (), sk, triple, hex_mode)},
            });
        }

        value_ptr make_section_value (database const & /*db*/, repo::bss_section const & section,
                                      repo::section_kind const sk, gsl::czstring const /*triple*/,
                                      bool const /*hex_mode*/) {
            (void) sk;
            PSTORE_ASSERT (sk == repo::section_kind::bss);
            return make_value (object::container{
                {"align", make_value (section.align ())},
                {"size", make_value (section.size ())},
            });
        }

        value_ptr make_fragment_value (database const & db, repo::fragment const & fragment,
                                       gsl::czstring const triple, bool const hex_mode) {

#define X(k)                                                                                       \
    case repo::section_kind::k:                                                                    \
        contents = make_section_value (db, fragment.at<repo::section_kind::k> (), kind, triple,    \
                                       hex_mode);                                                  \
        break;

            array::container array;
            for (repo::section_kind const kind : fragment) {
                PSTORE_ASSERT (fragment.has_section (kind));
                value_ptr contents;
                switch (kind) {
                    PSTORE_MCREPO_SECTION_KINDS
                case repo::section_kind::last:
                    PSTORE_ASSERT (false); //! OCLINT(PH - don't warn about the assert macro)
                    break;
                }
                array.emplace_back (make_value (
                    object::container{{"type", make_value (kind)}, {"contents", contents}}));
            }
            return make_value (std::move (array));
#undef X
        }

        value_ptr make_value (repo::linkage const l) {
#define X(a)                                                                                       \
    case (repo::linkage::a): name = #a; break;

            char const * name = "*unknown*";
            switch (l) { PSTORE_REPO_LINKAGES }
            return make_value (name);
#undef X
        }

        value_ptr make_value (repo::visibility const v) {
            char const * name = "*unknown*";
            switch (v) {
            case repo::visibility::default_vis: name = "default"; break;
            case repo::visibility::hidden_vis: name = "hidden"; break;
            case repo::visibility::protected_vis: name = "protected"; break;
            }
            return make_value (name);
        }

        value_ptr make_value (database const & db, repo::definition const & member) {
            using namespace serialize;
            using archive::make_reader;
            return make_value (object::container{
                {"digest", make_value (member.digest)},
                {"fext", make_value (member.fext)},
                {"name", make_value (indirect_string::read (db, member.name))},
                {"linkage", make_value (member.linkage ())},
                {"visibility", make_value (member.visibility ())},
            });
        }

        value_ptr make_value (database const & db,
                              std::shared_ptr<repo::compilation const> const & compilation) {
            array::container members;
            members.reserve (compilation->size ());
            for (auto const & member : *compilation) {
                members.emplace_back (make_value (db, member));
            }

            using namespace serialize;
            using archive::make_reader;
            return make_value (object::container{
                {"members", make_value (members)},
                {"path", make_value (indirect_string::read (db, compilation->path ()))},
                {"triple", make_value (indirect_string::read (db, compilation->triple ()))},
            });
        }

        value_ptr make_value (database const & db, index::fragment_index::value_type const & value,
                              gsl::czstring const triple, bool const hex_mode) {
            auto const fragment = repo::fragment::load (db, value.second);
            return make_value (object::container{
                {"digest", make_value (value.first)},
                {"fragment", make_fragment_value (db, *fragment, triple, hex_mode)}});
        }

        value_ptr make_value (database const & db,
                              index::compilation_index::value_type const & value) {
            auto const compilation = repo::compilation::load (db, value.second);
            return make_value (object::container{{"digest", make_value (value.first)},
                                                 {"compilation", make_value (db, compilation)}});
        }

        value_ptr make_value (database const & db,
                              index::debug_line_header_index::value_type const & value,
                              bool const hex_mode) {
            auto const debug_line_header = db.getro (value.second);
            return make_value (object::container{
                {"digest", make_value (value.first)},
                {"debug_line_header",
                 make_debuglineheader_value (debug_line_header.get (),
                                             debug_line_header.get () + value.second.size,
                                             hex_mode)}});
        }

    } // namespace dump
} // namespace pstore
