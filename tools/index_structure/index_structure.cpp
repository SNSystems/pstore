//*  _           _                 _                   _                   *
//* (_)_ __   __| | _____  __  ___| |_ _ __ _   _  ___| |_ _   _ _ __ ___  *
//* | | '_ \ / _` |/ _ \ \/ / / __| __| '__| | | |/ __| __| | | | '__/ _ \ *
//* | | | | | (_| |  __/>  <  \__ \ |_| |  | |_| | (__| |_| |_| | | |  __/ *
//* |_|_| |_|\__,_|\___/_/\_\ |___/\__|_|   \__,_|\___|\__|\__,_|_|  \___| *
//*                                                                        *
//===- tools/index_structure/index_structure.cpp --------------------------===//
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
#include <cstdlib>
#include <functional>
#include <iostream>

#include "pstore/config/config.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/hamt_map.hpp"
#include "pstore/core/hamt_map_types.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/core/sstring_view_archive.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/to_string.hpp"

#include "switches.hpp"

using pstore::database;
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
    dump_leaf (pstore::index::hamt_map<KeyType, ValueType, Hash, KeyEqual> const & index,
               std::ostream & os, pstore::address addr) {
        auto const this_id = "leaf" + pstore::to_string (addr.absolute ());
        auto const kvp = index.load_leaf_node (addr);

        std::ostringstream key_stream;
        key_stream << kvp.first;
        std::ostringstream value_stream;
        value_stream << kvp.second;

        os << this_id << " [shape=record label=\"" << escape (key_stream.str ()) << '|'
           << escape (value_stream.str ()) << "\"]\n";
        return this_id;
    }

    template <typename KeyType, typename Hash, typename KeyEqual>
    std::string dump_leaf (pstore::index::hamt_set<KeyType, Hash, KeyEqual> const & index,
                           std::ostream & os, pstore::address addr) {
        auto const this_id = "leaf" + pstore::to_string (addr.absolute ());

        std::ostringstream key_stream;
        key_stream << index.load_leaf_node (addr);

        os << this_id << " [shape=record label=\"" << escape (key_stream.str ()) << "\"]\n";
        return this_id;
    }


    template <typename IndexType>
    std::string dump (IndexType const & index, std::ostream & os, index_pointer node,
                      unsigned shifts);


    template <typename NodeType, typename IndexType>
    std::string dump_intermediate (IndexType const & index, std::ostream & os, index_pointer node,
                                   unsigned shifts) {
        assert (!node.is_heap ());
        auto const this_id =
            node_type_name<NodeType>::name + pstore::to_string (node.addr.absolute ());

        std::shared_ptr<void const> store_ptr;
        NodeType const * ptr = nullptr;
        std::tie (store_ptr, ptr) = NodeType::get_node (index.db (), node);
        assert (ptr != nullptr);

        for (auto const & child : *ptr) {
            auto const child_id = dump (index, os, child, shifts + hash_index_bits);
            os << this_id << " -> " << child_id << ";\n";
        }
        return this_id;
    }

    template <typename IndexType>
    std::string dump (IndexType const & index, std::ostream & os, index_pointer node,
                      unsigned shifts) {
        if (node.is_leaf ()) {
            assert (node.is_address ());
            return dump_leaf (index, os, node.addr);
        }
        return depth_is_internal_node (shifts)
                   ? dump_intermediate<internal_node> (index, os, node, shifts)
                   : dump_intermediate<linear_node> (index, os, node, shifts);
    }

    template <typename IndexType>
    void dump_index (IndexType const * const index, char const * name) {
        if (index == nullptr) {
            std::cerr << name << " index is empty\n";
            return;
        }

        if (index_pointer const root = index->root ()) {
            auto & os = std::cout;
            os << "digraph " << name << " {\ngraph [rankdir=LR];\n";
            auto label = dump (*index, os, root, 0U);
            os << "root -> " << label << '\n';
            os << "}\n";
        } else {
            std::cerr << name << " index is empty\n";
        }
    }

    template <pstore::trailer::indices Index>
    void dump_if_selected (switches const & opt, pstore::database & db) {
        if (opt.test (Index)) {
            char const * name =
                index_names[static_cast<std::underlying_type<pstore::trailer::indices>::type> (
                    Index)];
            dump_index (pstore::index::get_index<Index> (db, false /*create*/).get (), name);
        }
    }
} // anonymous namespace


#if defined(_WIN32) && !PSTORE_IS_INSIDE_LLVM
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

        dump_if_selected<pstore::trailer::indices::digest> (opt, db);
        dump_if_selected<pstore::trailer::indices::name> (opt, db);
        dump_if_selected<pstore::trailer::indices::write> (opt, db);
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, {
        std::cerr << "Error: " << ex.what () << '\n';
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
       std::cerr << "Unknown exception\n";
       exit_code = EXIT_FAILURE;
    })
    // clang-format on
    return exit_code;
}

