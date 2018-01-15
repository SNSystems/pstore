//*                                                   _             *
//*  _ __ ___   ___ _ __ ___ _ __   ___   __   ____ _| |_   _  ___  *
//* | '_ ` _ \ / __| '__/ _ \ '_ \ / _ \  \ \ / / _` | | | | |/ _ \ *
//* | | | | | | (__| | |  __/ |_) | (_) |  \ V / (_| | | |_| |  __/ *
//* |_| |_| |_|\___|_|  \___| .__/ \___/    \_/ \__,_|_|\__,_|\___| *
//*                         |_|                                     *
//===- lib/dump/mcrepo_value.cpp ------------------------------------------===//
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
#include "dump/mcrepo_value.hpp"

#include "dump/value.hpp"
#include "dump/db_value.hpp"
#include "pstore/db_archive.hpp"
#include "pstore/hamt_map.hpp"
#include "pstore/hamt_set.hpp"

namespace value {

#define X(a)                                                                                       \
    case (pstore::repo::section_type::a):                                                          \
        name = #a;                                                                                 \
        break;

    value_ptr make_value (pstore::repo::section_type t) {
        char const * name = "*unknown*";
        switch (t) { PSTORE_REPO_SECTION_TYPES }
        return make_value (name);
    }
#undef X

    value_ptr make_value (pstore::repo::internal_fixup const & ifx) {
        return make_value (object::container{
            {"section", make_value (ifx.section)},
            {"type", make_value (static_cast<std::uint64_t> (ifx.type))},
            {"offset", make_value (ifx.offset)},
            {"addend", make_value (ifx.addend)},
        });
    }

    value_ptr make_value (pstore::database & db, pstore::repo::external_fixup const & xfx) {
        pstore::serialize::archive::database_reader archive (db, xfx.name);
        std::string const name = pstore::serialize::read<std::string> (archive);

        return make_value (object::container{
            {"name", make_value (name)},
            {"type", make_value (static_cast<std::uint64_t> (xfx.type))},
            {"offset", make_value (xfx.offset)},
            {"addend", make_value (xfx.addend)},
        });
    }

    value_ptr make_value (pstore::database & db, pstore::repo::section const & section) {
        auto const & data = section.data ();
        auto const & ifixups = section.ifixups ();

        object::container v;
        v.emplace_back ("align", make_value (1U << section.align ()));
        v.emplace_back ("data", std::make_shared<binary> (std::begin (data), std::end (data)));
        v.emplace_back ("ifixups", make_value (std::begin (ifixups), std::end (ifixups)));

        array::container xfx_members;
        for (auto const & xfx : section.xfixups ()) {
            xfx_members.emplace_back (make_value (db, xfx));
        }
        v.emplace_back ("xfixups", make_value (xfx_members));
        return make_value (v);
    }

    value_ptr make_value (pstore::database & db, pstore::repo::fragment const & fragment) {
        array::container array;

        for (auto const key : fragment.sections ().get_indices ()) {
            auto const type = static_cast<pstore::repo::section_type> (key);
            array.emplace_back (make_value (object::container{
                {"type", make_value (type)}, {"contents", make_value (db, fragment[type])}}));
        }
        return make_value (std::move (array));
    }

    value_ptr make_fragments (pstore::database & db) {
        array::container result;
        if (pstore::index::digest_index * const digests =
                pstore::index::get_digest_index (db, false /* create */)) {

            array::container members;
            for (auto const & kvp : *digests) {
                auto fragment = pstore::repo::fragment::load (db, kvp.second);
                result.emplace_back (make_value (object::container{
                    {"digest", make_value (kvp.first)}, {"fragment", make_value (db, *fragment)}}));
            }
        }
        return make_value (result);
    }

#define X(a)                                                                                       \
    case (pstore::repo::linkage_type::a):                                                          \
        Name = #a;                                                                                 \
        break;

    value_ptr make_value (pstore::repo::linkage_type t) {
        char const * Name = "*unknown*";
        switch (t) { PSTORE_REPO_LINKAGE_TYPES }
        return make_value (Name);
    }
#undef X

    value_ptr make_value (pstore::database & db, pstore::repo::ticket_member const & member) {
        pstore::serialize::archive::database_reader archive (db, member.name);
        return make_value (object::container{
            {"digest", make_value (member.digest)},
            {"name", make_value (pstore::serialize::read<std::string> (archive))},
            {"linkage", make_value (member.linkage)},
        });
    }

    value_ptr make_value (pstore::database & db,
                          std::shared_ptr<pstore::repo::ticket const> ticket) {
        object::container v;
        array::container members;
        for (auto const & member : *ticket) {
            members.emplace_back (make_value (db, member));
        }
        v.emplace_back ("ticket_member", make_value (members));

        // Now try reading the ticket file path using a serializer.
        pstore::serialize::archive::database_reader archive (db, ticket->path ());
        auto path = pstore::serialize::read<std::string> (archive);

        return make_value (object::container{
            {"members", make_value (v)}, {"path", make_value (path)},
        });
    }

    value_ptr make_tickets (pstore::database & db) {
        array::container result;
        if (pstore::index::ticket_index * const tickets =
                pstore::index::get_ticket_index (db, false /* create */)) {

            for (auto const & kvp : *tickets) {
                auto const ticket = pstore::repo::ticket::load (db, kvp.second);
                result.emplace_back (make_value (object::container{
                    {"uuid", make_value (kvp.first)}, {"ticket", make_value (db, ticket)}}));
            }
        }
        return make_value (result);
    }

} // namespace value

// eof: lib/dump/mcrepo_value.cpp
