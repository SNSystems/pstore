//===- lib/exchange/export.cpp --------------------------------------------===//
//*                             _    *
//*   _____  ___ __   ___  _ __| |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| *
//* |  __/>  <| |_) | (_) | |  | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__| *
//*           |_|                    *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file export.cpp
/// \brief Implements the top level entry point for exporting a pstore database.

#include "pstore/exchange/export.hpp"

#include "pstore/core/generation_iterator.hpp"
#include "pstore/exchange/export_compilation.hpp"
#include "pstore/exchange/export_fragment.hpp"

namespace {

    // footers
    // ~~~~~~~
    decltype (auto) footers (pstore::database const & db) {
        auto const num_transactions = db.get_current_revision () + 1;
        std::vector<pstore::typed_address<pstore::trailer>> footers;
        footers.reserve (num_transactions);

        pstore::generation_container transactions{db};
        std::copy (std::begin (transactions), std::end (transactions),
                   std::back_inserter (footers));
        PSTORE_ASSERT (footers.size () == num_transactions);
        std::reverse (std::begin (footers), std::end (footers));
        return footers;
    }

    // emit debug line headers
    // ~~~~~~~~~~~~~~~~~~~~~~~
    bool emit_debug_line_headers (pstore::exchange::export_ns::ostream & os,
                                  pstore::exchange::export_ns::indent const ind,
                                  pstore::database const & db, unsigned const generation) {
        auto const debug_line_headers =
            pstore::index::get_index<pstore::trailer::indices::debug_line_header> (db);
        if (debug_line_headers->empty ()) {
            return false;
        }

        PSTORE_ASSERT (generation > 0);
        bool first = true;
        auto const object_indent = ind.next ();
        auto const out_fn = [&] (pstore::address const addr) {
            if (first) {
                os << ind << R"("debugline":{)";
                first = false;
            } else {
                os << ',';
            }
            os << '\n' << object_indent;
            auto const & kvp = debug_line_headers->load_leaf_node (db, addr);
            pstore::exchange::export_ns::emit_digest (os, kvp.first);
            os << R"(:")";
            std::shared_ptr<std::uint8_t const> const data = db.getro (kvp.second);
            auto const * const ptr = data.get ();
            pstore::to_base64 (ptr, ptr + kvp.second.size,
                               pstore::exchange::export_ns::ostream_inserter{os});
            os << '"';
        };
        pstore::diff (db, *debug_line_headers, generation - 1U,
                      pstore::exchange::export_ns::make_diff_out (&out_fn));
        if (!first) {
            os << '\n' << ind << "},\n";
        }
        return first;
    }

    std::string prefix (bool prev_emitted, pstore::exchange::export_ns::indent const ind,
                        std::string const & property) {
        return (prev_emitted ? ",\n" : "") + ind.str () + '"' + property + "\":";
    }

} // end anonymous namespace

namespace pstore {
    namespace exchange {
        namespace export_ns {

            void emit_database (database & db, ostream & os, bool const comments) {
                string_mapping string_table{db, name_index_tag ()};
                string_mapping path_table{db, path_index_tag ()};

                auto const ind = indent{}.next ();
                os << "{\n";
                os << ind << R"("version":1,)" << '\n';
                os << ind << R"("id":")" << db.get_header ().id ().str () << "\",\n";
                os << ind << R"("transactions":)";

                auto const f = footers (db);
                PSTORE_ASSERT (std::distance (std::begin (f), std::end (f)) >= 1);
                emit_array (
                    os, ind, std::next (std::begin (f)), std::end (f),
                    [&] (ostream & os1, indent const ind1,
                         pstore::typed_address<pstore::trailer> const footer_pos) {
                        auto const footer = db.getro (footer_pos);
                        unsigned const generation = footer->a.generation;
                        db.sync (generation);
                        os1 << ind1 << "{\n";
                        auto const object_indent = ind1.next ();
                        if (comments) {
                            os1 << object_indent << "// transaction #" << generation << '\n';
                        }
                        bool const names_emitted = emit_strings<trailer::indices::name> (
                            os1, object_indent, db, generation,
                            prefix (false, object_indent, "names"), &string_table, comments);
                        bool const paths_emitted = emit_strings<trailer::indices::path> (
                            os1, object_indent, db, generation,
                            prefix (names_emitted, object_indent, "paths"), &path_table, comments);
                        if (paths_emitted || names_emitted) {
                            os1 << ",\n";
                        }
                        if (emit_debug_line_headers (os1, object_indent, db, generation)) {
                            os1 << ",\n";
                        }
                        os1 << object_indent << R"("fragments":{)";
                        emit_fragments (os1, object_indent.next (), db, generation, string_table,
                                        comments);
                        os1 << '\n' << object_indent << "},\n";
                        os1 << object_indent << R"("compilations":{)";
                        emit_compilation_index (os1, object_indent.next (), db, generation,
                                                string_table, comments);
                        os1 << '\n' << object_indent << "}\n";
                        os1 << ind1 << '}';
                    });
                os << "\n}\n";
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore
