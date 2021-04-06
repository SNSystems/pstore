//===- unittests/exchange/test_linked_definitions_section.cpp -------------===//
//*  _ _       _            _       _       __ _       _ _   _                  *
//* | (_)_ __ | | _____  __| |   __| | ___ / _(_)_ __ (_) |_(_) ___  _ __  ___  *
//* | | | '_ \| |/ / _ \/ _` |  / _` |/ _ \ |_| | '_ \| | __| |/ _ \| '_ \/ __| *
//* | | | | | |   <  __/ (_| | | (_| |  __/  _| | | | | | |_| | (_) | | | \__ \ *
//* |_|_|_| |_|_|\_\___|\__,_|  \__,_|\___|_| |_|_| |_|_|\__|_|\___/|_| |_|___/ *
//*                                                                             *
//*                _   _              *
//*  ___  ___  ___| |_(_) ___  _ __   *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* \__ \  __/ (__| |_| | (_) | | | | *
//* |___/\___|\___|\__|_|\___/|_| |_| *
//*                                   *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_linked_definitions_section.hpp"
#include "pstore/exchange/export_section.hpp"

#include <iterator>
#include <sstream>

#include <gmock/gmock.h>

#include "pstore/exchange/export_fragment.hpp"
#include "pstore/exchange/import_fragment.hpp"
#include "pstore/json/json.hpp"

#include "empty_store.hpp"

using namespace pstore;

namespace {

    class LinkedDefinitionsSection : public testing::Test {
    public:
        LinkedDefinitionsSection ()
                : export_store_{}
                , export_db_{export_store_.file ()}
                , import_store_{}
                , import_db_{import_store_.file ()} {
            export_db_.set_vacuum_mode (database::vacuum_mode::disabled);
            import_db_.set_vacuum_mode (database::vacuum_mode::disabled);
        }

    protected:
        using transaction_lock = std::unique_lock<mock_mutex>;

        InMemoryStore export_store_;
        database export_db_;

        InMemoryStore import_store_;
        database import_db_;

        // Build and export a fragment which contains a linked-definitions section with the contents
        // supplied from the supplied pointer range.
        std::string export_fragment (repo::linked_definitions::value_type const * first,
                                     repo::linked_definitions::value_type const * last);
    };


    // Build and export a fragment which contains a linked-definitions section with the contents
    // supplied from the supplied pointer range.
    std::string LinkedDefinitionsSection::export_fragment (
        repo::linked_definitions::value_type const * const first,
        repo::linked_definitions::value_type const * const last) {
        mock_mutex mutex;
        auto transaction = begin (export_db_, transaction_lock{mutex});

        std::vector<std::unique_ptr<repo::section_creation_dispatcher>> dispatchers;
        dispatchers.emplace_back (new repo::linked_definitions_creation_dispatcher (first, last));
        auto const fext =
            repo::fragment::alloc (transaction, make_pointee_adaptor (dispatchers.begin ()),
                                   make_pointee_adaptor (dispatchers.end ()));

        transaction.commit ();

        exchange::export_ns::string_mapping exported_names{
            export_db_, pstore::exchange::export_ns::name_index_tag ()};
        exchange::export_ns::ostringstream str;
        emit_fragment (str, exchange::export_ns::indent{}, export_db_, exported_names,
                       export_db_.getro (fext), true);
        return str.str ();
    }

    template <typename ImportRule, typename... Args>
    decltype (auto) make_json_object_parser (database * const db, Args... args) {
        using namespace pstore::exchange::import_ns;
        return json::make_parser (callbacks::make<object_rule<ImportRule, Args...>> (db, args...));
    }


    void check_linked_definition (database const & db,
                                  repo::linked_definitions::value_type const & link) {
        // Find and load the compilation that link references.
        auto const compilation_index =
            pstore::index::get_index<pstore::trailer::indices::compilation> (db);
        auto const pos = compilation_index->find (db, link.compilation);
        ASSERT_NE (pos, compilation_index->end (db));

        std::shared_ptr<repo::compilation const> const compilation =
            repo::compilation::load (db, pos->second);
        ASSERT_LT (link.index, compilation->size ())
            << "index must lie within the number of definitions in this compilation";

        // Compute the offset of the link.index definition from the start of the compilation's
        // storage (c).
        auto const offset = reinterpret_cast<std::uintptr_t> (&(*compilation)[link.index]) -
                            reinterpret_cast<std::uintptr_t> (compilation.get ());
        // Compute the address of the link.index definition.
        auto const definition_address = pos->second.addr.to_address () + offset;
        ASSERT_EQ (definition_address, link.pointer.to_address ());
    }

} // end anonymous namespace

TEST_F (LinkedDefinitionsSection, RoundTripForPopulated) {
    using testing::ContainerEq;
    constexpr auto referenced_compilation_digest = pstore::index::digest{0x12345678, 0x9ABCDEF0};

    constexpr auto max_addr =
        typed_address<repo::definition>::make (std::numeric_limits<std::uint64_t>::max ());
    std::vector<repo::linked_definitions::value_type> exported_content{
        {referenced_compilation_digest, UINT32_C (0), max_addr},
        {referenced_compilation_digest, UINT32_C (1), max_addr},
    };

    // Build and export a fragment which contains a linked-definitions section with the contents
    // supplied from the export_content vector. The resulting JSON is in exported_json.
    std::string exported_json = this->export_fragment (
        exported_content.data (), exported_content.data () + exported_content.size ());

    // Now build the import database. First we we create a compilation that the linked-definition
    // fragment will later reference.
    {
        mock_mutex mutex;
        auto transaction = begin (import_db_, transaction_lock{mutex});
        {
            constexpr index::digest fragment_digest{0x9ABCDEF0, 0x12345678};
            pstore::extent<repo::fragment> const fext;
            constexpr auto str = pstore::typed_address<pstore::indirect_string>::make (0U);
            std::vector<repo::definition> definitions{
                {fragment_digest, fext, str, pstore::repo::linkage::external},
                {fragment_digest, fext, str, pstore::repo::linkage::external},
            };
            auto compilation_index =
                pstore::index::get_index<pstore::trailer::indices::compilation> (transaction.db ());
            compilation_index->insert (
                transaction, std::make_pair (referenced_compilation_digest,
                                             repo::compilation::alloc (transaction, str /*triple*/,
                                                                       std::begin (definitions),
                                                                       std::end (definitions))));
        }
        transaction.commit ();
    }

    constexpr auto imported_digest = pstore::index::digest{0x31415192, 0x9ABCDEF0};
    {
        mock_mutex mutex;
        auto transaction = begin (import_db_, transaction_lock{mutex});
        {
            exchange::import_ns::string_mapping imported_names;
            auto parser = make_json_object_parser<exchange::import_ns::fragment_sections> (
                &import_db_, &transaction, &imported_names, &imported_digest);
            parser.input (exported_json).eof ();
            ASSERT_FALSE (parser.has_error ())
                << "JSON error was: " << parser.last_error ().message () << ' '
                << parser.coordinate () << '\n'
                << exported_json;

            std::shared_ptr<exchange::import_ns::context> const & ctxt =
                parser.callbacks ().get_context ();
            ctxt->apply_patches (&transaction);
        }
        transaction.commit ();
    }

    auto fragments_index = index::get_index<trailer::indices::fragment> (import_db_);
    auto pos = fragments_index->find (import_db_, imported_digest);
    ASSERT_NE (pos, fragments_index->end (import_db_));
    auto imported_fragment = repo::fragment::load (import_db_, pos->second);
    ASSERT_TRUE (imported_fragment->has_section (repo::section_kind::linked_definitions));
    repo::linked_definitions const & linked =
        imported_fragment->at<repo::section_kind::linked_definitions> ();
    ASSERT_EQ (linked.size (), 2U);
    auto expos = exported_content.begin ();
    auto impos = linked.begin ();
    EXPECT_EQ (expos->compilation, impos->compilation);
    EXPECT_EQ (expos->index, impos->index);
    check_linked_definition (import_db_, *impos);
    ++expos;
    ++impos;
    EXPECT_EQ (expos->compilation, impos->compilation);
    EXPECT_EQ (expos->index, impos->index);
    check_linked_definition (import_db_, *impos);
}
