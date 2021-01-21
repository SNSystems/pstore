//*  _                             _   _              *
//* | |__  ___ ___   ___  ___  ___| |_(_) ___  _ __   *
//* | '_ \/ __/ __| / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* | |_) \__ \__ \ \__ \  __/ (__| |_| | (_) | | | | *
//* |_.__/|___/___/ |___/\___|\___|\__|_|\___/|_| |_| *
//*                                                   *
//===- unittests/exchange/test_bss_section.cpp ----------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_bss_section.hpp"
#include "pstore/exchange/export_section.hpp"

#include <iterator>
#include <sstream>

#include <gtest/gtest.h>

#include "pstore/exchange/import_names_array.hpp"
#include "pstore/exchange/import_section_to_importer.hpp"
#include "pstore/json/json.hpp"

#include "add_export_strings.hpp"
#include "compare_external_fixups.hpp"
#include "empty_store.hpp"
#include "section_helper.hpp"

namespace {

    class BssSection : public testing::Test {
    public:
        BssSection ()
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
        using namespace pstore::exchange::import;
        return pstore::json::make_parser (
            callbacks::make<object_rule<ImportRule, Args...>> (db, args...));
    }

} // end anonymous namespace

TEST_F (BssSection, RoundTripForAnEmptySection) {
    constexpr auto kind = pstore::repo::section_kind::bss;
    // The type used to store a text section's properties.
    using section_type = pstore::repo::enum_to_section_t<kind>;
    static_assert (std::is_same<section_type, pstore::repo::bss_section>::value,
                   "Expected bss to map to bss_section");

    pstore::exchange::export_ns::name_mapping exported_names{export_db_};
    pstore::repo::section_content exported_content{kind};
    std::string const exported_json =
        export_section<kind> (export_db_, exported_names, exported_content, false);



    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);

    pstore::exchange::import::name_mapping imported_names;
    pstore::repo::section_content imported_content;

    // Find the rule that is used to import sections represented by an instance of section_type.
    using section_importer =
        pstore::exchange::import::section_to_importer_t<section_type, decltype (inserter)>;
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

TEST_F (BssSection, RoundTripForPopulated) {
    constexpr auto kind = pstore::repo::section_kind::bss;
    // The type used to store a text section's properties.
    using section_type = pstore::repo::enum_to_section_t<kind>;
    static_assert (std::is_same<section_type, pstore::repo::bss_section>::value,
                   "Expected bss to map to bss_section");

    pstore::exchange::export_ns::name_mapping exported_names{export_db_};

    pstore::repo::section_content exported_content{kind};
    exported_content.align = 32U;
    exported_content.data.resize (7);

    std::string const exported_json =
        export_section<kind> (export_db_, exported_names, exported_content, false);



    // Parse the exported names JSON. The resulting index-to-string mappings are then available via
    // imported_names.
    pstore::exchange::import::name_mapping imported_names;

    // Now set up the import. We'll build two objects: an instance of a section-creation-dispatcher
    // which knows how to build a BSS section and a section-content which will describe the
    // contents of that new section.
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);

    pstore::repo::section_content imported_content;

    // Find the rule that is used to import sections represented by an instance of section_type.
    using section_importer =
        pstore::exchange::import::section_to_importer_t<section_type, decltype (inserter)>;
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

    EXPECT_EQ (imported_content.kind, exported_content.kind);
    EXPECT_EQ (imported_content.align, exported_content.align);
    EXPECT_THAT (imported_content.data, testing::ContainerEq (exported_content.data));
    EXPECT_TRUE (imported_content.ifixups.empty ());
    EXPECT_TRUE (imported_content.xfixups.empty ());
}

namespace {

    template <typename T>
    using not_null = pstore::gsl::not_null<T>;

    template <pstore::repo::section_kind Kind, typename OutputIterator>
    decltype (auto) parse (std::string const & src, pstore::database * const db,
                           pstore::exchange::import::name_mapping const & names,
                           OutputIterator * const inserter,                        // out
                           not_null<pstore::repo::section_content *> const content // out
    ) {

        // The type used to store the properties of a section of section-kind Kind.
        using section_type = typename pstore::repo::enum_to_section_t<Kind>;
        // Find the rule that is used to import sections represented by an instance of
        // section_type.
        using section_importer =
            pstore::exchange::import::section_to_importer_t<section_type, OutputIterator>;
        // Create a JSON parser which understands this section object.
        auto parser =
            make_json_object_parser<section_importer> (db, Kind, &names, content, inserter);
        // Parse the string.
        parser.input (src).eof ();
        return parser;
    }

    class BssSectionImport : public testing::Test {
    public:
        BssSectionImport ()
                : store_{}
                , db_{store_.file ()} {
            db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

    protected:
        InMemoryStore store_;
        pstore::database db_;
    };

} // end anonymous namespace

TEST_F (BssSectionImport, ZeroSizeSuccess) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    auto const & parser = parse<pstore::repo::section_kind::bss> (
        R"({ "align":8, "size":0 })", &db_, pstore::exchange::import::name_mapping{}, &inserter,
        &imported_content);
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();

    EXPECT_EQ (imported_content.kind, pstore::repo::section_kind::bss);
    EXPECT_EQ (imported_content.align, 8U);
    EXPECT_TRUE (imported_content.data.empty ());
    EXPECT_TRUE (imported_content.ifixups.empty ());
    EXPECT_TRUE (imported_content.xfixups.empty ());
}

TEST_F (BssSectionImport, MissingAlign) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    // The align value is missing.
    auto const & parser = parse<pstore::repo::section_kind::bss> (
        R"({ "size":16 })", &db_, pstore::exchange::import::name_mapping{}, &inserter,
        &imported_content);
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    EXPECT_EQ (imported_content.align, 1U);
    EXPECT_EQ (imported_content.data.size (), 16U);
}

TEST_F (BssSectionImport, BadAlignValue) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    // The align value must be a power of 2.
    auto const & parser = parse<pstore::repo::section_kind::bss> (
        R"({ "align":7, "size":16 })", &db_, pstore::exchange::import::name_mapping{}, &inserter,
        &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import::error::alignment_must_be_power_of_2));
}

TEST_F (BssSectionImport, BadAlignType) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    // The align value is a boolean rather than an integer.
    auto const & parser = parse<pstore::repo::section_kind::bss> (
        R"({ "align":true, "data":"" })", &db_, pstore::exchange::import::name_mapping{}, &inserter,
        &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import::error::unexpected_boolean));
}

TEST_F (BssSectionImport, MissingSize) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    // The data value is missing.
    auto const & parser = parse<pstore::repo::section_kind::bss> (
        R"({ "align":8 })", &db_, pstore::exchange::import::name_mapping{}, &inserter,
        &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import::error::bss_section_was_incomplete));
}

TEST_F (BssSectionImport, BadSizeType) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    // The data value is a boolean rather than a string.
    auto const & parser = parse<pstore::repo::section_kind::bss> (
        R"({ "align":8, "size":true })", &db_, pstore::exchange::import::name_mapping{}, &inserter,
        &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import::error::unexpected_boolean));
}
