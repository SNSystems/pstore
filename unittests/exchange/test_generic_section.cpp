//*                             _                      _   _              *
//*   __ _  ___ _ __   ___ _ __(_) ___   ___  ___  ___| |_(_) ___  _ __   *
//*  / _` |/ _ \ '_ \ / _ \ '__| |/ __| / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* | (_| |  __/ | | |  __/ |  | | (__  \__ \  __/ (__| |_| | (_) | | | | *
//*  \__, |\___|_| |_|\___|_|  |_|\___| |___/\___|\___|\__|_|\___/|_| |_| *
//*  |___/                                                                *
//===- unittests/exchange/test_generic_section.cpp ------------------------===//
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
#include "pstore/exchange/import_generic_section.hpp"
#include "pstore/exchange/export_section.hpp"

#include <sstream>

#include <gtest/gtest.h>

#include "pstore/exchange/import_names_array.hpp"
#include "pstore/exchange/import_section_to_importer.hpp"
#include "pstore/json/json.hpp"

#include "add_export_strings.hpp"
#include "compare_external_fixups.hpp"
#include "empty_store.hpp"

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

    /// placement_delete is paired with use of placement new and unique_ptr<> to enable unique_ptr<>
    /// to be used on memory that is not owned. unique_ptr<> will cause the object's destructor to
    /// be called when it goes out of scope, but the release of memory is a no-op.
    template <typename T>
    struct placement_delete {
        constexpr void operator() (T *) const noexcept {}
    };

    /// \tparam Kind The type of section to be built.
    /// \tparam SectionType  The type used to store the properties of Kind section.
    /// \tparam CreationDispatcher  The type of creation dispatcher used to instantiate sections of
    ///     type SectionType.
    template <pstore::repo::section_kind Kind,
              typename SectionType = typename pstore::repo::enum_to_section_t<Kind>,
              typename CreationDispatcher =
                  typename pstore::repo::section_to_creation_dispatcher<SectionType>::type>
    decltype (auto) build_section (pstore::gsl::not_null<std::vector<std::uint8_t> *> const buffer,
                                   pstore::repo::section_content const & content) {
        CreationDispatcher dispatcher{Kind, &content};
        buffer->resize (dispatcher.size_bytes ());
        auto * const data = buffer->data ();
        dispatcher.write (data);
        // Use unique_ptr<> to ensure that the section_type object's destructor is called. Since
        // the memory is not owned by this object (it belongs to 'buffer'), the delete operation
        // will do nothing thanks to placement_delete<>.
        return std::unique_ptr<SectionType const, placement_delete<SectionType const>>{
            reinterpret_cast<SectionType const *> (data), placement_delete<SectionType const>{}};
    }

    template <typename ImportRule, typename... Args>
    decltype (auto) make_json_object_parser (Args &&... args) {
        using rule = pstore::exchange::object_rule<ImportRule, Args...>;
        return pstore::json::make_parser (
            pstore::exchange::callbacks::make<rule> (std::forward<Args> (args)...));
    }

    template <pstore::repo::section_kind Kind>
    std::string export_section (pstore::database const & db,
                                pstore::exchange::export_name_mapping const & exported_names,
                                pstore::repo::section_content const & content) {
        // First build the section that we want to export.
        std::vector<std::uint8_t> buffer;
        auto const section = build_section<Kind> (&buffer, content);

        // Now export it.
        std::ostringstream os;
        pstore::exchange::export_section<Kind> (os, db, exported_names, *section);
        return os.str ();
    }

} // end anonymous namespace

TEST_F (GenericSection, RoundTripForAnEmptySection) {
    constexpr auto kind = pstore::repo::section_kind::text;
    // The type used to store a text section's properties.
    using section_type = pstore::repo::enum_to_section_t<kind>;
    static_assert (std::is_same<section_type, pstore::repo::generic_section>::value,
                   "Expected text to map to generic_section");

    pstore::exchange::export_name_mapping exported_names;
    pstore::repo::section_content exported_content;
    std::string const exported_json =
        export_section<kind> (export_db_, exported_names, exported_content);



    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);

    pstore::exchange::import_name_mapping imported_names;
    pstore::repo::section_content imported_content;

    // Find the rule that is used to import sections represented by an instance of section_type.
    using section_importer =
        pstore::exchange::section_to_importer_t<section_type, decltype (inserter)>;
    auto parser = make_json_object_parser<section_importer> (kind, import_db_, &imported_names,
                                                             &imported_content, &inserter);
    parser.input (exported_json).eof ();
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();

    ASSERT_EQ (dispatchers.size (), 1U)
        << "Expected a single creation dispatcher to be added to the dispatchers container";
    EXPECT_EQ (dispatchers.front ()->kind (), kind)
        << "The creation dispatcher should be able to create a text section";

    EXPECT_EQ (exported_content, imported_content);
}

TEST_F (GenericSection, RoundTripForPopulated) {
    constexpr auto kind = pstore::repo::section_kind::text;
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
    add_export_strings (export_db_, std::begin (names), std::end (names),
                        std::inserter (indir_strings, std::end (indir_strings)));

    // Write the names that we just created as JSON.
    pstore::exchange::export_name_mapping exported_names;
    std::ostringstream exported_names_stream;
    export_names (exported_names_stream, export_db_, export_db_.get_current_revision (),
                  &exported_names);


    pstore::repo::section_content exported_content;
    exported_content.align = 32U;
    {
        unsigned value = 0;
        std::generate_n (std::back_inserter (exported_content.data), 5U,
                         [&value] () { return value++; });
    }
    exported_content.ifixups.emplace_back (
        pstore::repo::section_kind::data, pstore::repo::relocation_type{3},
        std::uint64_t{5} /*offset*/, std::uint64_t{7} /*addend*/);
    exported_content.ifixups.emplace_back (
        pstore::repo::section_kind::read_only, pstore::repo::relocation_type{11},
        std::uint64_t{13} /*offset*/, std::uint64_t{17} /*addend*/);
    exported_content.xfixups.emplace_back (indir_strings[name1], pstore::repo::relocation_type{19},
                                           std::uint64_t{23} /*offset*/,
                                           std::uint64_t{29} /*addend*/);
    exported_content.xfixups.emplace_back (indir_strings[name2], pstore::repo::relocation_type{31},
                                           std::uint64_t{37} /*offset*/,
                                           std::uint64_t{41} /*addend*/);


    std::string const exported_json =
        export_section<kind> (export_db_, exported_names, exported_content);



    // Create matching names in the imported database.
    mock_mutex mutex;
    using transaction_lock = std::unique_lock<mock_mutex>;
    auto transaction = begin (import_db_, transaction_lock{mutex});

    // Parse the exported names JSON. The resulting index-to-string mappings are then available via
    // imported_names.
    pstore::exchange::import_name_mapping imported_names;
    {
        auto parser = pstore::json::make_parser (
            pstore::exchange::callbacks::make<pstore::exchange::array_rule<
                pstore::exchange::names_array_members<transaction_lock>, decltype (&transaction),
                decltype (&imported_names)>> (&transaction, &imported_names));
        parser.input (exported_names_stream.str ()).eof ();
        ASSERT_FALSE (parser.has_error ())
            << "Expected the JSON parse to succeed (" << parser.last_error ().message () << ')';
    }

    // Now set up the import. We'll build two objects: an instance of a section-creation-dispatcher
    // which knows how to build a text section and a section-content which will describe the
    // contents of that new section.
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);

    pstore::repo::section_content imported_content;

    // Find the rule that is used to import sections represented by an instance of section_type.
    using section_importer =
        pstore::exchange::section_to_importer_t<section_type, decltype (inserter)>;
    auto parser = make_json_object_parser<section_importer> (kind, import_db_, &imported_names,
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

    transaction.commit ();
}

namespace {

    template <typename T>
    using not_null = pstore::gsl::not_null<T>;

    class GenericSectionImport : public testing::Test {
    public:
        GenericSectionImport ()
                : store_{}
                , db_{store_.file ()} {
            db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

        template <pstore::repo::section_kind Kind, typename OutputIterator>
        decltype (auto) parse (std::string const & src,
                               pstore::exchange::import_name_mapping const & names,
                               OutputIterator * const inserter,
                               not_null<pstore::repo::section_content *> const content) {

            // The type used to store the properties of a section of section-kind Kind.
            using section_type = typename pstore::repo::enum_to_section_t<Kind>;
            // Find the rule that is used to import sections represented by an instance of
            // section_type.
            using section_importer =
                pstore::exchange::section_to_importer_t<section_type, OutputIterator>;
            // Create a JSON parser which understands this section object.
            auto parser =
                make_json_object_parser<section_importer> (Kind, db_, &names, content, inserter);
            parser.input (src).eof ();
            return parser;
        }

        InMemoryStore store_;
        pstore::database db_;
    };

} // end anonymous namespace

TEST_F (GenericSectionImport, TextEmptySuccess) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    auto const & parser = this->parse<pstore::repo::section_kind::text> (
        R"({ "align":8, "data":"" })", pstore::exchange::import_name_mapping{}, &inserter,
        &imported_content);
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

    auto const & parser = this->parse<pstore::repo::section_kind::text> (
        R"({ "data":"" })", pstore::exchange::import_name_mapping{}, &inserter, &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_error::generic_section_was_incomplete));
}

TEST_F (GenericSectionImport, TextBadAlignValue) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    auto const & parser = this->parse<pstore::repo::section_kind::text> (
        R"({ "align":7, "data":"" })", pstore::exchange::import_name_mapping{}, &inserter,
        &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_error::alignment_must_be_power_of_2));
}

TEST_F (GenericSectionImport, TextBadAlignType) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    auto const & parser = this->parse<pstore::repo::section_kind::text> (
        R"({ "align":true, "data":"" })", pstore::exchange::import_name_mapping{}, &inserter,
        &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_error::unexpected_boolean));
}

TEST_F (GenericSectionImport, TextMissingData) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    auto const & parser = this->parse<pstore::repo::section_kind::text> (
        R"({ "align":8 })", pstore::exchange::import_name_mapping{}, &inserter, &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_error::generic_section_was_incomplete));
}

TEST_F (GenericSectionImport, TextBadDataType) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    auto const & parser = this->parse<pstore::repo::section_kind::text> (
        R"({ "align":8, "data":true })", pstore::exchange::import_name_mapping{}, &inserter,
        &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_error::unexpected_boolean));
}

TEST_F (GenericSectionImport, TextBadDataContent) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    auto const & parser = this->parse<pstore::repo::section_kind::text> (
        R"({ "align":8, "data":"this is not ASCII85" })", pstore::exchange::import_name_mapping{},
        &inserter, &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_error::bad_base64_data));
}

TEST_F (GenericSectionImport, TextBadInternalFixupsType) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    auto const & parser = this->parse<pstore::repo::section_kind::text> (
        R"({ "align":8, "data":"", "ifixups":true })", pstore::exchange::import_name_mapping{},
        &inserter, &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_error::unexpected_boolean));
}

TEST_F (GenericSectionImport, TextBadExternalFixupsType) {
    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto inserter = std::back_inserter (dispatchers);
    pstore::repo::section_content imported_content;

    auto const & parser = this->parse<pstore::repo::section_kind::text> (
        R"({ "align":8, "data":"", "xfixups":true })", pstore::exchange::import_name_mapping{},
        &inserter, &imported_content);
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_error::unexpected_boolean));
}
