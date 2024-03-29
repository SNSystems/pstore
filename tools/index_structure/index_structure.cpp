//===- tools/index_structure/index_structure.cpp --------------------------===//
//*  _           _                 _                   _                   *
//* (_)_ __   __| | _____  __  ___| |_ _ __ _   _  ___| |_ _   _ _ __ ___  *
//* | | '_ \ / _` |/ _ \ \/ / / __| __| '__| | | |/ __| __| | | | '__/ _ \ *
//* | | | | | (_| |  __/>  <  \__ \ |_| |  | |_| | (__| |_| |_| | | |  __/ *
//* |_|_| |_|\__,_|\___/_/\_\ |___/\__|_|   \__,_|\___|\__|\__,_|_|  \___| *
//*                                                                        *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>

#include "pstore/config/config.hpp"
#include "pstore/command_line/tchar.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/hamt_map.hpp"
#include "pstore/core/hamt_map_types.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/sstring_view_archive.hpp"
#include "pstore/support/portab.hpp"

#include "switches.hpp"

using pstore::database;
// Note that using these internal "details" namespace is not, in general, good practice.
// Here it is justified because the tool's purpose is to poke about inside the index
// internals!
using pstore::index::details::depth_is_internal_node;
using pstore::index::details::hash_index_bits;
using pstore::index::details::index_pointer;
using pstore::index::details::internal_node;
using pstore::index::details::linear_node;

namespace {
    template <typename NodeType>
    struct node_type_name {};

    template <>
    struct node_type_name<internal_node> {
        static char const * name;
    };
    char const * node_type_name<internal_node>::name = "internal";

    template <>
    struct node_type_name<linear_node> {
        static char const * name;
    };
    char const * node_type_name<linear_node>::name = "linear";



    std::string escape (std::string const & str) {
        std::string result;
        result.reserve (str.length ());
        for (auto c : str) {
            switch (c) {
            case '{':
            case '}':
            case '|':
            case '"': result += '\\'; PSTORE_FALLTHROUGH;
            default: result += c; break;
            }
        }
        return result;
    }

    template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
    std::string
    dump_leaf (pstore::database const & db,
               pstore::index::hamt_map<KeyType, ValueType, Hash, KeyEqual> const & index,
               std::ostream & os, pstore::address addr) {
        auto const this_id = "leaf" + std::to_string (addr.absolute ());
        auto const kvp = index.load_leaf_node (db, addr);

        std::ostringstream key_stream;
        key_stream << kvp.first;
        std::ostringstream value_stream;
        value_stream << kvp.second;

        os << this_id << " [shape=record label=\"" << escape (key_stream.str ()) << '|'
           << escape (value_stream.str ()) << "\"]\n";
        return this_id;
    }

    template <typename KeyType, typename Hash, typename KeyEqual>
    std::string dump_leaf (pstore::database const & db,
                           pstore::index::hamt_set<KeyType, Hash, KeyEqual> const & index,
                           std::ostream & os, pstore::address addr) {
        auto const this_id = "leaf" + std::to_string (addr.absolute ());

        std::ostringstream key_stream;
        key_stream << index.load_leaf_node (db, addr);

        os << this_id << " [shape=record label=\"" << escape (key_stream.str ()) << "\"]\n";
        return this_id;
    }


    template <typename IndexType>
    std::string dump (pstore::database const & db, IndexType const & index, std::ostream & os,
                      index_pointer node, unsigned shifts);


    template <typename NodeType, typename IndexType>
    std::string dump_intermediate (pstore::database const & db, IndexType const & index,
                                   std::ostream & os, index_pointer node, unsigned shifts) {
        PSTORE_ASSERT (!node.is_heap ());
        auto const this_id =
            node_type_name<NodeType>::name + std::to_string (node.to_address ().absolute ());

        std::shared_ptr<void const> store_ptr;
        NodeType const * ptr = nullptr;
        std::tie (store_ptr, ptr) = NodeType::get_node (db, node);
        PSTORE_ASSERT (ptr != nullptr);

        for (auto const & child : *ptr) {
            auto const child_id =
                dump (db, index, os, index_pointer{child}, shifts + hash_index_bits);
            os << this_id << " -> " << child_id << ";\n";
        }
        return this_id;
    }

    template <typename IndexType>
    std::string dump (pstore::database const & db, IndexType const & index, std::ostream & os,
                      index_pointer node, unsigned shifts) {
        if (node.is_leaf ()) {
            PSTORE_ASSERT (node.is_address ());
            return dump_leaf (db, index, os, node.to_address ());
        }
        return depth_is_internal_node (shifts)
                   ? dump_intermediate<internal_node> (db, index, os, node, shifts)
                   : dump_intermediate<linear_node> (db, index, os, node, shifts);
    }

    template <typename IndexType>
    void dump_index (pstore::database const & db, IndexType const * const index,
                     char const * name) {
        if (index == nullptr) {
            std::cerr << name << " index is empty\n";
            return;
        }

        if (index_pointer const root = index->root ()) {
            auto & os = std::cout;
            os << "digraph " << name << " {\ngraph [rankdir=LR];\n";
            auto label = dump (db, *index, os, root, 0U);
            os << "root -> " << label << '\n';
            os << "}\n";
        } else {
            std::cerr << name << " index is empty\n";
        }
    }

    template <pstore::trailer::indices Index>
    struct index_name {};
#define X(a)                                                                                       \
    template <>                                                                                    \
    struct index_name<pstore::trailer::indices::a> {                                               \
        static constexpr auto name = #a;                                                           \
    };
    PSTORE_INDICES
#undef X

    template <pstore::trailer::indices Index>
    void dump_if_selected (switches const & opt, pstore::database const & db) {
        if (opt.test (Index)) {
            auto const index = pstore::index::get_index<Index> (db, false /*create*/);
            dump_index (db, index.get (), index_name<Index>::name);
        }
    }

} // end anonymous namespace


#if defined(_WIN32)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    PSTORE_TRY {
        switches opt;
        std::tie (opt, exit_code) = get_switches (argc, argv);
        if (exit_code != EXIT_SUCCESS) {
            return exit_code;
        }

        pstore::database db (opt.db_path, pstore::database::access_mode::read_only);
        db.sync (opt.revision);

#define X(a) dump_if_selected<pstore::trailer::indices::a> (opt, db);
        PSTORE_INDICES
#undef X
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, { // clang-format on
        pstore::command_line::error_stream << PSTORE_NATIVE_TEXT ("Error: ")
                                           << pstore::utf::to_native_string (ex.what ())
                                           << PSTORE_NATIVE_TEXT ('\n');
        exit_code = EXIT_FAILURE;
    })
    // clang-format off
    PSTORE_CATCH (..., { // clang-format on
        pstore::command_line::error_stream << PSTORE_NATIVE_TEXT ("Unknown exception\n");
        exit_code = EXIT_FAILURE;
    })
    // clang-format on
    return exit_code;
}
