//*                                                   _             *
//*  _ __ ___   ___ _ __ ___ _ __   ___   __   ____ _| |_   _  ___  *
//* | '_ ` _ \ / __| '__/ _ \ '_ \ / _ \  \ \ / / _` | | | | |/ _ \ *
//* | | | | | | (__| | |  __/ |_) | (_) |  \ V / (_| | | |_| |  __/ *
//* |_| |_| |_|\___|_|  \___| .__/ \___/    \_/ \__,_|_|\__,_|\___| *
//*                         |_|                                     *
//===- lib/dump/mcrepo_value.cpp ------------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc. 
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
namespace value {

#define X(a)                                                                                       \
    case (pstore::repo::SectionType::a):                                                           \
        Name = #a;                                                                                 \
        break;
    value_ptr make_value (pstore::repo::SectionType t) {
        char const * Name = "*unknown*";
        switch (t) { PSTORE_REPO_SECTION_TYPES }
        return make_value (Name);
    }
#undef X

    value_ptr make_value (pstore::repo::InternalFixup const & ifx) {
        return make_value (object::container{
            {"section", make_value (ifx.Section)},
            {"type", make_value (static_cast<std::uint64_t> (ifx.Type))},
            {"offset", make_value (ifx.Offset)},
            {"addend", make_value (ifx.Addend)},
        });
    }

    value_ptr make_value (pstore::database & db, pstore::repo::ExternalFixup const & xfx) {
        pstore::serialize::archive::database_reader archive (db, xfx.Name);
        std::string const name = pstore::serialize::read<std::string> (archive);

        return make_value (object::container{
            {"name", make_value (name)},
            {"type", make_value (static_cast<std::uint64_t> (xfx.Type))},
            {"offset", make_value (xfx.Offset)},
            {"addend", make_value (xfx.Addend)},
        });
    }

    value_ptr make_value (pstore::database & db, pstore::repo::Section const & section) {
        auto const & data = section.data ();
        auto const & ifixups = section.ifixups ();

        object::container v;
        v.emplace_back ("data", std::make_shared<binary> (std::begin (data), std::end (data)));
        v.emplace_back ("ifixups", make_value (std::begin (ifixups), std::end (ifixups)));

        array::container xfx_members;
        for (auto const & xfx : section.xfixups ()) {
            xfx_members.emplace_back (make_value (db, xfx));
        }
        v.emplace_back ("xfixups", make_value (xfx_members));
        return make_value (v);
    }

    value_ptr make_value (pstore::database & db, pstore::repo::Fragment const & fragment) {
        array::container array;

        for (auto const Key : fragment.sections().getIndices ()) {
            auto const type = static_cast<pstore::repo::SectionType>(Key);
            array.emplace_back (make_value (object::container{
                {"type", make_value (type)},
                {"contents", make_value (db, fragment[type])}
            }));
        }
        return make_value (std::move (array));
    }

    value_ptr make_fragments (pstore::database & db) {
        array::container result;
        if (pstore::index::digest_index * const digests =
                db.get_digest_index (false /* create */)) {

            array::container members;
            for (auto const & kvp : *digests) {
                auto F =
                    std::static_pointer_cast<pstore::repo::Fragment const> (db.getro (kvp.second));
                result.emplace_back (make_value (object::container{
                    {"digest", make_value (kvp.first)},
                    {"fragment", make_value (db, *F)}
                }));
            }
        }
        return make_value (result);
    }

#define X(a)                                                                                       \
    case (pstore::repo::linkage_type::a):                                                           \
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
        std::string const Name = pstore::serialize::read<std::string> (archive);
        auto const Linkage = static_cast<pstore::repo::linkage_type> (member.linkage);

        return make_value (object::container{
            {"digest", make_value (member.digest)},
            {"name", make_value (Name)},
            {"linkage", make_value (Linkage)},
            {"comdat", make_value (member.comdat)},
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
		pstore::serialize::archive::database_reader archive(db, ticket->path());
		auto path = pstore::serialize::read<std::string>(archive);

        return make_value (object::container{
            {"members", make_value (v)},
            {"path", make_value (path)},
        });
    }

    value_ptr make_tickets (pstore::database & db) {
        array::container result;
        if (pstore::index::ticket_index * const tickets =
                db.get_ticket_index (false /* create */)) {

            for (auto const & kvp : *tickets) {
                auto T = pstore::repo::ticket::get_ticket (db, kvp.second);
                result.emplace_back (make_value (
                    object::container{{"uuid", make_value (kvp.first)},
                                      {"ticket", make_value (db, T)}}));
            }
        }
        return make_value (result);
    }

} // namespace value

// eof: lib/dump/mcrepo_value.cpp
