//*  _           _                 _        _        *
//* (_)_ __   __| | _____  __  ___| |_ __ _| |_ ___  *
//* | | '_ \ / _` |/ _ \ \/ / / __| __/ _` | __/ __| *
//* | | | | | (_| |  __/>  <  \__ \ || (_| | |_\__ \ *
//* |_|_| |_|\__,_|\___/_/\_\ |___/\__\__,_|\__|___/ *
//*                                                  *
//===- tools/index_stats/index_stats.cpp ----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file main.cpp

#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/modifiers.hpp"
#include "pstore/command_line/revision_opt.hpp"
#include "pstore/command_line/tchar.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/file_header.hpp"
#include "pstore/core/hamt_map.hpp"
#include "pstore/core/hamt_map_types.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/core/index_types.hpp"

using namespace pstore;

namespace {

    command_line::opt<command_line::revision_opt, command_line::parser<std::string>> revision{
        "revision", command_line::desc ("The starting revision number (or 'HEAD')")};
    command_line::alias revision2{"r", command_line::desc ("Alias for --revision"),
                                  command_line::aliasopt (revision)};

    command_line::opt<std::string> db_path{command_line::positional, command_line::required,
                                           command_line::usage ("repository"),
                                           command_line::desc ("Database path")};

} // end anonymous namespace

namespace {

    using index_pointer = index::details::index_pointer;
    using internal_node = index::details::internal_node;
    using linear_node = index::details::linear_node;
    constexpr auto max_internal_depth = index::details::max_internal_depth;

    struct stats {
    public:
        explicit stats (database const & db) noexcept;

        template <typename Key, typename Hash, typename KeyEqual>
        void traverse (index::hamt_set<Key, Hash, KeyEqual> const & index) {
            traverse (index.root ());
        }

        template <typename Key, typename Value, typename Hash, typename KeyEqual>
        void traverse (index::hamt_map<Key, Value, Hash, KeyEqual> const & index) {
            traverse (index.root ());
        }

        double branching_factor () const noexcept {
            return internal_visited_ == 0 ? 0.0
                                          : static_cast<double> (internal_out_edges_) /
                                                static_cast<double> (internal_visited_);
        }
        double mean_leaf_depth () const noexcept {
            return leaves_visited_ == 0
                       ? 0.0
                       : static_cast<double> (leaf_depth_) / static_cast<double> (leaves_visited_);
        }
        unsigned max_depth () const noexcept { return max_depth_; }

    private:
        database const & db_;

        std::uint64_t internal_out_edges_ = 0;
        std::size_t internal_visited_ = 0;
        std::uint64_t leaf_depth_ = 0;
        std::size_t leaves_visited_ = 0;
        unsigned max_depth_ = 0;

        void traverse (index_pointer root);
        void traverse (index_pointer node, unsigned depth);
        void visit_linear (linear_node const & linear);
        void visit_leaf_node (unsigned depth);
        void visit_internal (internal_node const & internal, unsigned depth);
    };

    stats::stats (database const & db) noexcept
            : db_{db} {}

    void stats::traverse (index_pointer root) {
        if (!root) {
            return;
        }
        traverse (root, 1U);
    }

    void stats::traverse (index_pointer node, unsigned depth) {
        max_depth_ = std::max (max_depth_, depth);
        if (depth >= max_internal_depth && node.is_linear ()) {
            auto const linear = linear_node::get_node (db_, node);
            return this->visit_linear (*linear.second);
        }
        if (node.is_internal ()) {
            auto const internal = internal_node::get_node (db_, node);
            return this->visit_internal (*internal.second, depth);
        }
        return this->visit_leaf_node (depth);
    }

    void stats::visit_linear (linear_node const & linear) {
        internal_out_edges_ += linear.size ();
        ++internal_visited_;
    }

    void stats::visit_leaf_node (unsigned depth) {
        leaf_depth_ += depth;
        ++leaves_visited_;
    }

    void stats::visit_internal (internal_node const & internal, unsigned depth) {
        internal_out_edges_ += internal.size ();
        ++internal_visited_;
        for (index_pointer child : internal) {
            this->traverse (child, depth + 1U);
        }
    }

    template <trailer::indices Index>
    struct index_name {};
#define X(a)                                                                                       \
    template <>                                                                                    \
    struct index_name<trailer::indices::a> {                                               \
        static constexpr auto name = #a;                                                           \
    };
    PSTORE_INDICES
#undef X

    template <trailer::indices Index>
    void dump_index_stats (database const & db) {
        if (auto index = index::get_index<Index> (db, false /*create*/)) {
            stats s{db};
            s.traverse (*index);

            static constexpr auto newline = NATIVE_TEXT ("\n");
            static constexpr auto comma = NATIVE_TEXT (",");
            command_line::out_stream << utf::to_native_string (index_name<Index>::name) << comma
                                     << s.branching_factor () << comma << s.mean_leaf_depth ()
                                     << comma << s.max_depth () << comma << index->size ()
                                     << newline;
        }
    }

} // namespace

#if defined(_WIN32)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    PSTORE_TRY {
        command_line::parse_command_line_options (
            argc, argv, "Dumps statistics for the indexes in a pstore database");

        database db{db_path.get (), database::access_mode::read_only};
        db.sync (static_cast<unsigned> (revision.get ()));

        command_line::out_stream << NATIVE_TEXT (
            "name,branching-factor,mean-leaf-depth,max-depth,size\n");
#define X(a) dump_index_stats<trailer::indices::a> (db);
        PSTORE_INDICES
#undef X
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, { // clang-format on
        command_line::error_stream << NATIVE_TEXT ("Error: ") << utf::to_native_string (ex.what ())
                                   << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format off
    PSTORE_CATCH (..., { // clang-format on
        command_line::error_stream << NATIVE_TEXT ("Unknown error.") << std::endl;
        exit_code = EXIT_FAILURE;
    })
    return exit_code;
}
