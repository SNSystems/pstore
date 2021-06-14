//===- unittests/exchange/test_generic_section.cpp ------------------------===//
//*                             _                      _   _              *
//*   __ _  ___ _ __   ___ _ __(_) ___   ___  ___  ___| |_(_) ___  _ __   *
//*  / _` |/ _ \ '_ \ / _ \ '__| |/ __| / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* | (_| |  __/ | | |  __/ |  | | (__  \__ \  __/ (__| |_| | (_) | | | | *
//*  \__, |\___|_| |_|\___|_|  |_|\___| |___/\___|\___|\__|_|\___/|_| |_| *
//*  |___/                                                                *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_generic_section.hpp"
#include "pstore/exchange/export_section.hpp"

#include <iterator>
#include <sstream>

#include <gtest/gtest.h>

#include "pstore/exchange/import_section_to_importer.hpp"
#include "pstore/exchange/import_strings_array.hpp"
#include "pstore/json/json.hpp"

#include "add_export_strings.hpp"
#include "compare_external_fixups.hpp"
#include "empty_store.hpp"
#include "section_helper.hpp"

namespace {

    class GenericSection : public testing::Test {
    public:
        GenericSection ()
                : export_store_{}
                , export_db_{export_store_.file ()}
                , import_store_{}
                , import_db_{import_store_.file ()} {
            export_db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
            import_db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

        using transaction_lock = std::unique_lock<mock_mutex>;

        InMemoryStore export_store_;
        pstore::database export_db_;

        InMemoryStore import_store_;
        pstore::database import_db_;
    };

    template <typename ImportRule, typename... Args>
    decltype (auto) make_json_object_parser (pstore::database * const db, Args... args) {
        using namespace pstore::exchange::import_ns;
        return pstore::json::make_parser (
            callbacks::make<object_rule<ImportRule, Args...>> (db, args...));
    }

} // end anonymous namespace

TEST_F (GenericSection, RoundTripForAnEmptySection) {
    using namespace pstore::exchange;

    constexpr auto kind = pstore::repo::section_kind::text;
    // The type used to store a text section's properties.
    using section_type = pstore::repo::enum_to_section_t<kind>;
    static_assert (std::is_same<section_type, pstore::repo::generic_section>::value,
                   "Expected text to map to generic_section");

    export_ns::string_mapping exported_names{export_db_, export_ns::name_index_tag ()};
    pstore::repo::section_content exported_content;
    std::string const exported_json =
        export_section<kind> (export_db_, exported_names, exported_content, false);



    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);

    import_ns::string_mapping imported_names;
    pstore::repo::section_content imported_content;

    // Find the rule that is used to import sections represented by an instance of section_type.
    using section_importer = import_ns::section_to_importer_t<section_type, decltype (inserter)>;
    auto parser = make_json_object_parser<section_importer> (&import_db_, kind, &imported_names,
                                                             &imported_content, &inserter);
    parser.input (exported_json).eof ();
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ()
                                       << ' ' << parser.coordinate () << '\n'
                                       << exported_json;

    ASSERT_EQ (dispatchers.size (), 1U)
        << "Expected a single creation dispatcher to be added to the dispatchers container";
    EXPECT_EQ (dispatchers.front ()->kind (), kind)
        << "The creation dispatcher should be able to create a text section";

    EXPECT_EQ (exported_content, imported_content);
}

TEST_F (GenericSection, RoundTripForPopulated) {
    using namespace pstore::exchange;

    constexpr auto kind = pstore::repo::section_kind::text;
    constexpr auto names_index = pstore::trailer::indices::name;
    constexpr auto comments = false;

    // The type used to store a text section's properties.
    using section_type = pstore::repo::enum_to_section_t<kind>;
    static_assert (std::is_same<section_type, pstore::repo::generic_section>::value,
                   "Expected text to map to generic_section");

    // Add names to the store so that external fixups can use then. add_export_strings()
    // yields a mapping from each name to its indirect-address.
    constexpr auto * name1 = "name1";
    constexpr auto * name2 = "name2";
    std::array<pstore::gsl::czstring, 2> names{{name1, name2}};
    std::unordered_map<std::string, pstore::typed_address<pstore::indirect_string>> indir_strings;
    add_export_strings<names_index> (export_db_, std::begin (names), std::end (names),
                                     std::inserter (indir_strings, std::end (indir_strings)));

    // Write the names that we just created as JSON.
    export_ns::string_mapping exported_names{export_db_, export_ns::name_index_tag ()};
    export_ns::ostringstream exported_names_stream;
    export_ns::emit_strings<names_index> (exported_names_stream, export_ns::indent{}, export_db_,
                                          export_db_.get_current_revision (), "", &exported_names,
                                          comments);


    pstore::repo::section_content exported_content;
    exported_content.align = 32U;
    {
        unsigned value = 0;
        std::generate_n (std::back_inserter (exported_content.data), 5U, [&value] () {
            return static_cast<decltype (exported_content.data)::value_type> (value++);
        });
    }
    exported_content.ifixups.emplace_back (pstore::repo::section_kind::data,
                                           pstore::repo::relocation_type{3},
                                           std::uint64_t{5} /*offset*/, std::int64_t{7} /*addend*/);
    exported_content.ifixups.emplace_back (
        pstore::repo::section_kind::read_only, pstore::repo::relocation_type{11},
        std::uint64_t{13} /*offset*/, std::int64_t{17} /*addend*/);
    exported_content.xfixups.emplace_back (indir_strings[name1], pstore::repo::relocation_type{19},
                                           pstore::repo::reference_strength::strong,
                                           std::uint64_t{23} /*offset*/,
                                           std::int64_t{29} /*addend*/);
    exported_content.xfixups.emplace_back (indir_strings[name2], pstore::repo::relocation_type{31},
                                           pstore::repo::reference_strength::weak,
                                           std::uint64_t{37} /*offset*/,
                                           std::int64_t{41} /*addend*/);


    std::string const exported_json =
        export_section<kind> (export_db_, exported_names, exported_content, comments);



    // Parse the exported names JSON. The resulting index-to-string mappings are then available via
    // imported_names.
    import_ns::string_mapping imported_names;
    {
        // Create matching names in the imported database.
        mock_mutex mutex;
        using transaction_lock = std::unique_lock<mock_mutex>;
        auto transaction = begin (import_db_, transaction_lock{mutex});

        auto parser = pstore::json::make_parser (
            import_ns::callbacks::make<
                import_ns::array_rule<import_ns::strings_array_members, decltype (&transaction),
                                      decltype (&imported_names)>> (&import_db_, &transaction,
                                                                    &imported_names));
        parser.input (exported_names_stream.str ()).eof ();
        ASSERT_FALSE (parser.has_error ())
            << "Expected the JSON parse to succeed (" << parser.last_error ().message () << ')';

        transaction.commit ();
    }

    // Now set up the import. We'll build two objects: an instance of a section-creation-dispatcher
    // which knows how to build a text section and a section-content which will describe the
    // contents of that new section.
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);

    pstore::repo::section_content imported_content;

    // Find the rule that is used to import sections represented by an instance of section_type.
    using section_importer = import_ns::section_to_importer_t<section_type, decltype (inserter)>;
    auto parser = make_json_object_parser<section_importer> (&import_db_, kind, &imported_names,
                                                             &imported_content, &inserter);
    parser.input (exported_json).eof ();
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();

    ASSERT_EQ (dispatchers.size (), 1U)
        << "Expected a single creation dispatcher to be added to the dispatchers container";
    EXPECT_EQ (dispatchers.front ()->kind (), kind)
        << "The creation dispatcher should be able to create a text section";

    EXPECT_EQ (exported_content.kind, imported_content.kind);
    EXPECT_EQ (exported_content.align, imported_content.align);
    EXPECT_THAT (exported_content.data, testing::ContainerEq (imported_content.data));
    EXPECT_THAT (exported_content.ifixups, testing::ContainerEq (imported_content.ifixups));
    compare_external_fixups (export_db_, exported_content.xfixups, import_db_,
                             imported_content.xfixups);
}

namespace {

    template <typename T>
    using not_null = pstore::gsl::not_null<T>;

    template <pstore::repo::section_kind Kind, typename OutputIterator>
    decltype (auto) parse (std::string const & src, not_null<pstore::database *> const db,
                           pstore::exchange::import_ns::string_mapping const & names,
                           OutputIterator * const inserter,                        // out
                           not_null<pstore::repo::section_content *> const content // out
    ) {

        // The type used to store the properties of a section of section-kind Kind.
        using section_type = typename pstore::repo::enum_to_section_t<Kind>;
        // Find the rule that is used to import sections represented by an instance of
        // section_type.
        using section_importer =
            pstore::exchange::import_ns::section_to_importer_t<section_type, OutputIterator>;
        // Create a JSON parser which understands this section object.
        auto parser =
            make_json_object_parser<section_importer> (db, Kind, &names, content, inserter);
        // Parse the string.
        parser.input (src).eof ();
        return parser;
    }

    class GenericSectionImport : public testing::Test {
    public:
        GenericSectionImport ()
                : store_{}
                , db_{store_.file ()} {
            db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

    protected:
        InMemoryStore store_;
        pstore::database db_;
    };

} // end anonymous namespace

TEST_F (GenericSectionImport, TextEmptySuccess) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    auto const & parser = parse<pstore::repo::section_kind::text> (
        R"({ "align":8, "data":"" })", &db_, pstore::exchange::import_ns::string_mapping{},
        &inserter, &imported_content);
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();

    EXPECT_EQ (imported_content.kind, pstore::repo::section_kind::text);
    EXPECT_EQ (imported_content.align, 8U);
    EXPECT_TRUE (imported_content.data.empty ());
    EXPECT_TRUE (imported_content.ifixups.empty ());
    EXPECT_TRUE (imported_content.xfixups.empty ());
}

TEST_F (GenericSectionImport, TextMissingAlign) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    // The align value is missing.
    auto const & parser = parse<pstore::repo::section_kind::text> (
        R"({ "data":"" })", &db_, pstore::exchange::import_ns::string_mapping{}, &inserter,
        &imported_content);
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    EXPECT_EQ (imported_content.align, 1U);
    EXPECT_TRUE (imported_content.data.empty ());
}

TEST_F (GenericSectionImport, TextBadAlignValue) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    // The align value must be a power of 2.
    auto const & parser = parse<pstore::repo::section_kind::text> (
        R"({ "align":7, "data":"" })", &db_, pstore::exchange::import_ns::string_mapping{},
        &inserter, &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_ns::error::alignment_must_be_power_of_2));
}

TEST_F (GenericSectionImport, TextBadAlignType) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    // The align value is a boolean rather than an integer.
    auto const & parser = parse<pstore::repo::section_kind::text> (
        R"({ "align":true, "data":"" })", &db_, pstore::exchange::import_ns::string_mapping{},
        &inserter, &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_ns::error::unexpected_boolean));
}

TEST_F (GenericSectionImport, TextMissingData) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    // The data value is missing.
    auto const & parser = parse<pstore::repo::section_kind::text> (
        R"({ "align":8 })", &db_, pstore::exchange::import_ns::string_mapping{}, &inserter,
        &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (
        parser.last_error (),
        make_error_code (pstore::exchange::import_ns::error::generic_section_was_incomplete));
}

TEST_F (GenericSectionImport, TextBadDataType) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    // The data value is a boolean rather than a string.
    auto const & parser = parse<pstore::repo::section_kind::text> (
        R"({ "align":8, "data":true })", &db_, pstore::exchange::import_ns::string_mapping{},
        &inserter, &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_ns::error::unexpected_boolean));
}

TEST_F (GenericSectionImport, TextBadDataContent) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    // The data string does not contain valid ASCII85-encoded data.
    auto const & parser = parse<pstore::repo::section_kind::text> (
        R"({ "align":8, "data":"this is not ASCII85" })", &db_,
        pstore::exchange::import_ns::string_mapping{}, &inserter, &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_ns::error::bad_base64_data));
}

TEST_F (GenericSectionImport, TextBadInternalFixupsType) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    // The ifixups value is a boolean rather than an array.
    auto const & parser = parse<pstore::repo::section_kind::text> (
        R"({ "align":8, "data":"", "ifixups":true })", &db_,
        pstore::exchange::import_ns::string_mapping{}, &inserter, &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_ns::error::unexpected_boolean));
}

TEST_F (GenericSectionImport, TextBadExternalFixupsType) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    // The xfixups value is a boolean rather than an array.
    auto const & parser = parse<pstore::repo::section_kind::text> (
        R"({ "align":8, "data":"", "xfixups":true })", &db_,
        pstore::exchange::import_ns::string_mapping{}, &inserter, &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_ns::error::unexpected_boolean));
}
