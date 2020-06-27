//*                             _    *
//*   _____  ___ __   ___  _ __| |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| *
//* |  __/>  <| |_) | (_) | |  | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__| *
//*           |_|                    *
//===- lib/exchange/export.cpp --------------------------------------------===//
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
#include "pstore/exchange/export.hpp"

#include <type_traits>
#include <unordered_map>

#include "pstore/core/index_types.hpp"
#include "pstore/diff/diff.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/core/generation_iterator.hpp"
#include "pstore/adt/sstring_view.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/indirect_string.hpp"
#include "pstore/mcrepo/section.hpp"
#include "pstore/mcrepo/generic_section.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/support/base64.hpp"
#include "pstore/exchange/export_emit.hpp"


using namespace pstore::exchange;

namespace {

    class stdio_output : public std::iterator<std::output_iterator_tag, char> {
    public:
        stdio_output & operator= (char c) {
            std::fputc (c, stdout);
            return *this;
        }
        stdio_output & operator* () noexcept { return *this; }
        stdio_output & operator++ () noexcept { return *this; }
        stdio_output operator++ (int) noexcept { return *this; }
    };

    constexpr bool comments = false;

    decltype (auto) footers (pstore::database const & db) {
        auto const num_transactions = db.get_current_revision () + 1;
        std::vector<pstore::typed_address<pstore::trailer>> footers;
        footers.reserve (num_transactions);

        pstore::generation_container transactions{db};
        std::copy (std::begin (transactions), std::end (transactions),
                   std::back_inserter (footers));
        assert (footers.size () == num_transactions);
        std::reverse (std::begin (footers), std::end (footers));
        return footers;
    }

    std::string get_string (pstore::database const & db,
                            pstore::typed_address<pstore::indirect_string> addr) {
        return pstore::serialize::read<pstore::indirect_string> (
                   pstore::serialize::archive::database_reader{db, addr.to_address ()})
            .to_string ();
    }

    void show_string (pstore::database const & db,
                      pstore::typed_address<pstore::indirect_string> addr) {
        if (comments) {
            std::printf ("  // \"%s\"", get_string (db, addr).c_str ());
        }
    }


    class name_mapping {
    public:
        void add (pstore::address addr) {
            assert (names_.find (addr) == names_.end ());
            assert (names_.size () <= std::numeric_limits<std::uint64_t>::max ());
            names_[addr] = static_cast<std::uint64_t> (names_.size ());
        }
        std::uint64_t index (pstore::address addr) const {
            auto const pos = names_.find (addr);
            assert (pos != names_.end ());
            return pos->second;
        }
        std::uint64_t index (pstore::typed_address<pstore::indirect_string> addr) const {
            return index (addr.to_address ());
        }

    private:
        std::unordered_map<pstore::address, std::uint64_t> names_;
    };


    template <typename Iterator, typename Function>
    void emit_array (crude_ostream & os, Iterator first, Iterator last,
                     pstore::gsl::czstring indent, Function fn) {
        auto sep = "\n";
        auto tail_sep = "";
        auto tail_sep_indent = "";
        os << "[";
        std::for_each (first, last, [&] (auto const & element) {
            os << sep;
            fn (element);
            sep = ",\n";
            tail_sep = "\n";
            tail_sep_indent = indent;
        });
        os << tail_sep << tail_sep_indent << "]";
    }

#define INDENT1 "  "
#define INDENT2 INDENT1 INDENT1
#define INDENT3 INDENT2 INDENT1
#define INDENT4 INDENT3 INDENT1
#define INDENT5 INDENT4 INDENT1
#define INDENT6 INDENT5 INDENT1
#define INDENT7 INDENT6 INDENT1
#define INDENT8 INDENT7 INDENT1


    void names (crude_ostream & os, pstore::database const & db, unsigned const generation,
                name_mapping * const string_table) {

        auto strings = pstore::index::get_index<pstore::trailer::indices::name> (db);
        auto const container = pstore::diff::diff (db, *strings, generation - 1U);
        emit_array (os, std::begin (container), std::end (container), INDENT3,
                    [&] (pstore::address addr) {
                        pstore::indirect_string const str = strings->load_leaf_node (db, addr);
                        pstore::shared_sstring_view owner;
                        pstore::raw_sstring_view view = str.as_db_string_view (&owner);
                        os << INDENT4;
                        pstore::exchange::emit_string (os, view);
                        string_table->add (addr);
                    });
    }

    pstore::gsl::czstring section_name (pstore::repo::section_kind section) {
        auto result = "unknown";
#define X(a)                                                                                       \
    case pstore::repo::section_kind::a: result = #a; break;
        switch (section) {
            PSTORE_MCREPO_SECTION_KINDS
        case pstore::repo::section_kind::last: break;
        }
#undef X
        return result;
    }

    void
    emit_section_ifixups (crude_ostream & os,
                          pstore::repo::container<pstore::repo::internal_fixup> const & ifixups) {
        os << INDENT6 "\"ifixups\": ";
        emit_array (os, std::begin (ifixups), std::end (ifixups), INDENT6,
                    [&] (pstore::repo::internal_fixup const & ifx) {
                        os << INDENT7 "{\n";
                        os << INDENT8 "\"section\": \"" << section_name (ifx.section) << ",\n";
                        os << INDENT8 "\"type\": " << static_cast<unsigned> (ifx.type) << ",\n";
                        os << INDENT8 "\"offset\": " << ifx.offset << ",\n";
                        os << INDENT8 "\"addend\": " << ifx.addend << "\n";
                        os << INDENT7 "}";
                    });
    }

    void
    emit_section_xfixups (crude_ostream & os, pstore::database const & db,
                          name_mapping const & names,
                          pstore::repo::container<pstore::repo::external_fixup> const & xfixups) {
        os << INDENT6 "\"xfixups\": ";
        emit_array (os, std::begin (xfixups), std::end (xfixups), INDENT6,
                    [&] (pstore::repo::external_fixup const & xfx) {
                        os << INDENT7 "{\n";
                        os << INDENT8 "\"name\": " << names.index (xfx.name) << ",";
                        show_string (db, xfx.name);
                        os << "\n";
                        os << INDENT8 "\"type\": " << static_cast<unsigned> (xfx.type) << ",\n";
                        os << INDENT8 "\"offset\": " << xfx.offset << ",\n";
                        os << INDENT8 "\"addend\": " << xfx.addend << "\n";
                        os << INDENT7 "}";
                    });
    }

    template <pstore::repo::section_kind Kind,
              typename Content = typename pstore::repo::enum_to_section<Kind>::type>
    void emit_section (crude_ostream & os, pstore::database const & db, name_mapping const & names,
                       Content const & content) {
        {
            os << INDENT6 "\"data\": \"";
            pstore::repo::container<std::uint8_t> const payload = content.payload ();
            pstore::to_base64 (std::begin (payload), std::end (payload), stdio_output{});
            os << "\",\n";
        }
        os << INDENT6 "\"align\": " << content.align () << ",\n";
        emit_section_ifixups (os, content.ifixups ());
        os << ",\n";
        emit_section_xfixups (os, db, names, content.xfixups ());
        os << "\n";
    }

    template <>
    void emit_section<pstore::repo::section_kind::bss, pstore::repo::bss_section> (
        crude_ostream & os, pstore::database const & db, name_mapping const & names,
        pstore::repo::bss_section const & bss) {

        os << INDENT6 "\"size\": " << bss.size () << ",\n";
        os << INDENT6 "\"align\": " << bss.align () << ",\n";
        emit_section_ifixups (os, bss.ifixups ());
        os << ",\n";
        emit_section_xfixups (os, db, names, bss.xfixups ());
        os << "\n";
    }

    template <>
    void emit_section<pstore::repo::section_kind::debug_line, pstore::repo::debug_line_section> (
        crude_ostream & os, pstore::database const & /*db*/, name_mapping const & /*names*/,
        pstore::repo::debug_line_section const & dl) {

        assert (dl.align () == 1U);
        assert (dl.xfixups ().size () == 0U);

        // TODO: emit the header digest.
        pstore::index::digest header_digest;
        os << INDENT6 "\"header\": \"" << header_digest.to_hex_string () << "\",\n";

        {
            os << INDENT6 "\"data\": \"";
            pstore::repo::container<std::uint8_t> const payload = dl.payload ();
            pstore::to_base64 (std::begin (payload), std::end (payload), stdio_output{});
            os << "\",\n";
        }

        emit_section_ifixups (os, dl.ifixups ());
        os << "\n";
    }

    template <>
    void emit_section<pstore::repo::section_kind::dependent, pstore::repo::dependents> (
        crude_ostream & os, pstore::database const & /*db*/, name_mapping const & /*names*/,
        pstore::repo::dependents const & dependents) {

        emit_array (os, std::begin (dependents), std::end (dependents), INDENT6,
                    [&] (pstore::typed_address<pstore::repo::compilation_member> const & d) {
                        // FIXME: represent as a Relative JSON Pointer
                        // (https://json-schema.org/draft/2019-09/relative-json-pointer.html).
                    });
    }

    void fragments (crude_ostream & os, pstore::database const & db, unsigned const generation,
                    name_mapping const & names) {
        auto fragments = pstore::index::get_index<pstore::trailer::indices::fragment> (db);
        if (!fragments->empty ()) {
            auto fragment_sep = "\n";
            assert (generation > 0U);
            for (pstore::address const & addr :
                 pstore::diff::diff (db, *fragments, generation - 1U)) {
                auto const & kvp = fragments->load_leaf_node (db, addr);
                os << fragment_sep << INDENT4 "\"" << kvp.first.to_hex_string () << "\": {\n";

                auto fragment = db.getro (kvp.second);
                auto section_sep = INDENT5;
                for (pstore::repo::section_kind section : *fragment) {
                    os << section_sep << '"' << section_name (section) << "\": {\n";
#define X(a)                                                                                       \
    case pstore::repo::section_kind::a:                                                            \
        emit_section<pstore::repo::section_kind::a> (                                              \
            os, db, names, fragment->at<pstore::repo::section_kind::a> ());                        \
        break;
                    switch (section) {
                        PSTORE_MCREPO_SECTION_KINDS
                    case pstore::repo::section_kind::last:
                        // llvm_unreachable...
                        assert (false);
                        break;
                    }
#undef X
                    os << INDENT5 "}";
                    section_sep = ",\n" INDENT5;
                }
                os << "\n" INDENT4 "}";
                fragment_sep = ",\n";
            }
        }
    }

#define X(a)                                                                                       \
    case pstore::repo::linkage::a: return os << #a;
    crude_ostream & operator<< (crude_ostream & os, pstore::repo::linkage linkage) {
        switch (linkage) { PSTORE_REPO_LINKAGES }
        return os << "unknown";
    }
#undef X

    crude_ostream & operator<< (crude_ostream & os, pstore::repo::visibility visibility) {
        switch (visibility) {
        case pstore::repo::visibility::default_vis: return os << "default";
        case pstore::repo::visibility::hidden_vis: return os << "hidden";
        case pstore::repo::visibility::protected_vis: return os << "protected";
        }
        return os << "unknown";
    }

    void compilations (crude_ostream & os, pstore::database const & db, unsigned const generation,
                       name_mapping const & names) {
        auto compilations = pstore::index::get_index<pstore::trailer::indices::compilation> (db);
        if (!compilations->empty ()) {
            auto sep = "\n";
            assert (generation > 0);
            for (pstore::address const & addr :
                 pstore::diff::diff (db, *compilations, generation - 1U)) {
                auto const & kvp = compilations->load_leaf_node (db, addr);
                auto compilation = db.getro (kvp.second);

                os << sep << INDENT4 "\"" << kvp.first.to_hex_string () << "\": {";
                os << "\n" INDENT5 "\"path\": " << names.index (compilation->path ()) << ',';
                show_string (db, compilation->path ());
                os << "\n" INDENT5 "\"triple\": " << names.index (compilation->triple ()) << ',';
                show_string (db, compilation->triple ());
                os << "\n" INDENT5 "\"definitions\": ";
                emit_array (os, compilation->begin (), compilation->end (), INDENT5,
                            [&] (pstore::repo::compilation_member const & d) {
                                os << INDENT6 "{\n";
                                os << INDENT7 "\"digest\": \"" << d.digest.to_hex_string ()
                                   << "\",\n";
                                os << INDENT7 "\"name\": " << names.index (d.name) << ',';
                                show_string (db, d.name);
                                os << '\n';
                                os << INDENT7 "\"linkage\": \"" << d.linkage () << "\",\n";
                                os << INDENT7 "\"visibility\": \"" << d.visibility () << "\"\n";
                                os << INDENT6 "}";
                            });
                os << "\n" INDENT4 "}";
                sep = ",\n";
            }
        }
    }

    void debug_line (crude_ostream & os, pstore::database const & db, unsigned const generation) {
        auto debug_line_headers =
            pstore::index::get_index<pstore::trailer::indices::debug_line_header> (db);
        if (!debug_line_headers->empty ()) {
            auto sep = "\n";
            assert (generation > 0);
            for (pstore::address const & addr :
                 pstore::diff::diff (db, *debug_line_headers, generation - 1U)) {
                auto const & kvp = debug_line_headers->load_leaf_node (db, addr);
                os << sep << INDENT4 << '"' << kvp.first.to_hex_string () << "\": \"";
                std::shared_ptr<std::uint8_t const> const data = db.getro (kvp.second);
                auto const ptr = data.get ();
                pstore::to_base64 (ptr, ptr + kvp.second.size, stdio_output{});
                os << '"';

                sep = ",\n";
            }
        }
    }

} // end anonymous namespace

namespace pstore {
    namespace exchange {

        void export_database (database & db, crude_ostream & os) {
            name_mapping string_table;
            os << "{\n";
            os << INDENT1 "\"version\": 1,\n";
            os << INDENT1 "\"transactions\": ";

            auto const f = footers (db);
            assert (std::distance (std::begin (f), std::end (f)) >= 1);
            emit_array (os, std::next (std::begin (f)), std::end (f), INDENT1,
                        [&] (pstore::typed_address<pstore::trailer> footer_pos) {
                            auto const footer = db.getro (footer_pos);
                            unsigned const generation = footer->a.generation;
                            db.sync (generation);
                            os << INDENT2 "{\n";
                            if (comments) {
                                os << INDENT3 "// generation " << generation << '\n';
                            }
                            os << INDENT3 "\"names\": ";
                            names (os, db, generation, &string_table);
                            os << ",\n" INDENT3 "\"debugline\": {";
                            debug_line (os, db, generation);
                            os << "\n" INDENT3 "},\n";
                            os << INDENT3 "\"fragments\": {";
                            fragments (os, db, generation, string_table);
                            os << "\n" INDENT3 "},\n";
                            os << INDENT3 "\"compilations\": {";
                            compilations (os, db, generation, string_table);
                            os << "\n" INDENT3 "}\n";
                            os << INDENT2 "}";
                        });
            os << "\n}\n";
        }

    } // end namespace exchange
} // end namespace pstore
