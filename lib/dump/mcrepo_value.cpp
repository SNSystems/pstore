//===- lib/dump/mcrepo_value.cpp ------------------------------------------===//
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
#include "pstore/dump/mcrepo_value.hpp"

#include "pstore/config/config.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/dump/db_value.hpp"
#include "pstore/dump/mcdebugline_value.hpp"
#include "pstore/dump/mcdisassembler_value.hpp"
#include "pstore/dump/value.hpp"

namespace {


    pstore::dump::value_ptr make_relocation_type_value (pstore::repo::relocation_type const rt,
                                                        pstore::dump::parameters const & parm) {
        (void) parm;
#ifdef PSTORE_IS_INSIDE_LLVM
#    define ELF_RELOC(name, value)                                                                 \
    case value: str = #name; break;

        if (parm.triple.getArch () == llvm::Triple::x86_64) {
            auto str = "*unknown*";
            switch (rt) {
#    include "llvm/BinaryFormat/ELFRelocs/x86_64.def"
            }
            return pstore::dump::make_value (str);
        }

#    undef ELF_RELOC
#endif // PSTORE_IS_INSIDE_LLVM
        return pstore::dump::make_value (static_cast<std::uint64_t> (rt));
    }

} // end anonymous namespace

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

        value_ptr make_value (repo::internal_fixup const & ifx, parameters const & parm) {
            auto const v = std::make_shared<object> (object::container{
                {"section", make_value (ifx.section)},
                {"type", make_relocation_type_value (ifx.type, parm)},
                {"offset", make_value (ifx.offset)},
                {"addend", make_value (ifx.addend)},
            });
            v->compact ();
            return std::static_pointer_cast<value> (v);
        }

        value_ptr make_value (repo::external_fixup const & xfx, parameters const & parm) {
            using serialize::read;
            using serialize::archive::make_reader;

            return make_value (object::container{
                {"name", make_value (indirect_string::read (parm.db, xfx.name))},
                {"type", make_relocation_type_value (xfx.type, parm)},
                {"is_weak", make_value (xfx.is_weak)},
                {"offset", make_value (xfx.offset)},
                {"addend", make_value (xfx.addend)},
            });
        }

        value_ptr make_section_value (repo::generic_section const & section,
                                      repo::section_kind const sk, parameters const & parm) {
            (void) sk;
            auto const & payload = section.payload ();
            value_ptr data_value;
#ifdef PSTORE_IS_INSIDE_LLVM
            if (sk == repo::section_kind::text) {
                std::uint8_t const * const first = payload.data ();
                data_value = make_disassembled_value (first, first + payload.size (), parm);
            }
#endif // PSTORE_IS_INSIDE_LLVM
            if (!data_value) {
                if (parm.hex_mode) {
                    data_value =
                        std::make_shared<binary16> (std::begin (payload), std::end (payload));
                } else {
                    data_value =
                        std::make_shared<binary> (std::begin (payload), std::end (payload));
                }
            }
            repo::container<repo::internal_fixup> const internal_fixups = section.ifixups ();
            repo::container<repo::external_fixup> const external_fixups = section.xfixups ();
            return make_value (object::container{
                {"align", make_value (section.align ())},
                {"data", data_value},
                {"ifixups", make_value (std::begin (internal_fixups), std::end (internal_fixups),
                                        [&] (repo::internal_fixup const & ifx) {
                                            return make_value (ifx, parm);
                                        })},
                {"xfixups", make_value (std::begin (external_fixups), std::end (external_fixups),
                                        [&] (repo::external_fixup const & xfx) {
                                            return make_value (xfx, parm);
                                        })},
            });
        }

        value_ptr make_section_value (repo::linked_definitions const & linked_definitions,
                                      repo::section_kind const /*sk*/, parameters const & parm) {
            return make_value (
                std::begin (linked_definitions), std::end (linked_definitions),
                [&] (repo::linked_definitions::value_type const & member) {
                    return make_value (object::container{
                        {"compilation", make_value (member.compilation)},
                        {"index", make_value (member.index)},
                        {"pointer", make_value (*parm.db.getro (member.pointer), parm)},
                    });
                });
        }

        value_ptr make_section_value (repo::debug_line_section const & section,
                                      repo::section_kind const sk, parameters const & parm) {
            PSTORE_ASSERT (sk == repo::section_kind::debug_line);
            return make_value (object::container{
                {"digest", make_value (section.header_digest ())},
                {"header", make_value (section.header_extent ())},
                {"generic", make_section_value (section.generic (), sk, parm)},
            });
        }

        value_ptr make_section_value (repo::bss_section const & section,
                                      repo::section_kind const sk, parameters const & /*parm*/) {
            (void) sk;
            PSTORE_ASSERT (sk == repo::section_kind::bss);
            return make_value (object::container{
                {"align", make_value (section.align ())},
                {"size", make_value (section.size ())},
            });
        }

        value_ptr make_fragment_value (repo::fragment const & fragment, parameters const & parm) {

#define X(k)                                                                                       \
    case repo::section_kind::k:                                                                    \
        contents = make_section_value (fragment.at<repo::section_kind::k> (), kind, parm);         \
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

        value_ptr make_value (repo::definition const & member, parameters const & parm) {
            using namespace serialize;
            using archive::make_reader;
            return make_value (object::container{
                {"digest", make_value (member.digest)},
                {"fext", make_value (member.fext)},
                {"name", make_value (indirect_string::read (parm.db, member.name))},
                {"linkage", make_value (member.linkage ())},
                {"visibility", make_value (member.visibility ())},
            });
        }

        value_ptr make_value (std::shared_ptr<repo::compilation const> const & compilation,
                              parameters const & parm) {
            array::container members;
            members.reserve (compilation->size ());
            for (auto const & member : *compilation) {
                members.emplace_back (make_value (member, parm));
            }

            using namespace serialize;
            using archive::make_reader;
            return make_value (object::container{
                {"members", make_value (members)},
                {"path", make_value (indirect_string::read (parm.db, compilation->path ()))},
                {"triple", make_value (indirect_string::read (parm.db, compilation->triple ()))},
            });
        }

        value_ptr make_value (index::fragment_index::value_type const & value,
                              parameters const & parm) {
            auto const fragment = repo::fragment::load (parm.db, value.second);
            return make_value (
                object::container{{"digest", make_value (value.first)},
                                  {"fragment", make_fragment_value (*fragment, parm)}});
        }

        value_ptr make_value (index::compilation_index::value_type const & value,
                              parameters const & parm) {
            auto const compilation = repo::compilation::load (parm.db, value.second);
            return make_value (object::container{{"digest", make_value (value.first)},
                                                 {"compilation", make_value (compilation, parm)}});
        }

        value_ptr make_value (index::debug_line_header_index::value_type const & value,
                              parameters const & parm) {
            auto const debug_line_header = parm.db.getro (value.second);
            return make_value (object::container{
                {"digest", make_value (value.first)},
                {"debug_line_header",
                 make_debuglineheader_value (debug_line_header.get (),
                                             debug_line_header.get () + value.second.size,
                                             parm.hex_mode)}});
        }

    } // namespace dump
} // namespace pstore
