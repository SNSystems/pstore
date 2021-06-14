//===- unittests/exchange/test_section_fixups.cpp -------------------------===//
//*                _   _                __ _                       *
//*  ___  ___  ___| |_(_) ___  _ __    / _(_)_  ___   _ _ __  ___  *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  | |_| \ \/ / | | | '_ \/ __| *
//* \__ \  __/ (__| |_| | (_) | | | | |  _| |>  <| |_| | |_) \__ \ *
//* |___/\___|\___|\__|_|\___/|_| |_| |_| |_/_/\_\\__,_| .__/|___/ *
//*                                                    |_|         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/export_fixups.hpp"
#include "pstore/exchange/import_fixups.hpp"

#include <sstream>
#include <vector>

#include "pstore/adt/sstring_view.hpp"
#include "pstore/json/json.hpp"
#include "pstore/exchange/export_ostream.hpp"
#include "pstore/exchange/import_error.hpp"
#include "pstore/exchange/import_rule.hpp"
#include "pstore/exchange/import_non_terminals.hpp"
#include "pstore/exchange/import_strings_array.hpp"

#include <gmock/gmock.h>

#include "add_export_strings.hpp"
#include "compare_external_fixups.hpp"

namespace {

    using internal_fixup_collection = std::vector<pstore::repo::internal_fixup>;
    using internal_fixup_array_root =
        pstore::exchange::import_ns::array_rule<pstore::exchange::import_ns::ifixups_object,
                                                pstore::exchange::import_ns::string_mapping *,
                                                internal_fixup_collection *>;

    class ExchangeSectionFixups : public ::testing::Test {
    public:
        ExchangeSectionFixups ()
                : db_storage_{}
                , db_{db_storage_.file ()} {
            db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

    private:
        InMemoryStore db_storage_;

    protected:
        pstore::database db_;
    };

} // end anonymous namespace

TEST_F (ExchangeSectionFixups, RoundTripInternalEmpty) {
    // Start with an empty collection of internal fixups.
    internal_fixup_collection ifixups;

    // Export the internal fixup array to the 'os' string-stream.
    pstore::exchange::export_ns::ostringstream os;
    emit_internal_fixups (os, pstore::exchange::export_ns::indent{}, std::begin (ifixups),
                          std::end (ifixups));

    // Setup the parse.
    pstore::exchange::import_ns::string_mapping names;
    internal_fixup_collection imported_ifixups;
    auto parser = pstore::json::make_parser (
        pstore::exchange::import_ns::callbacks::make<internal_fixup_array_root> (
            &db_, &names, &imported_ifixups));

    // Import the data that we just exported.
    parser.input (os.str ());
    parser.eof ();

    // Check the result.
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    EXPECT_THAT (imported_ifixups, testing::ContainerEq (ifixups))
        << "The imported and exported ifixups should match";
}

TEST_F (ExchangeSectionFixups, RoundTripInternalCollection) {
    // Start with a small collection of internal fixups.
    internal_fixup_collection ifixups;
    ifixups.emplace_back (pstore::repo::section_kind::text, pstore::repo::relocation_type{17},
                          std::uint64_t{19} /*offset */, std::int64_t{23} /*addend*/);
    ifixups.emplace_back (pstore::repo::section_kind::text, pstore::repo::relocation_type{29},
                          std::uint64_t{31} /*offset */, std::int64_t{37} /*addend*/);
    ifixups.emplace_back (pstore::repo::section_kind::thread_data,
                          pstore::repo::relocation_type{41}, std::uint64_t{43} /*offset */,
                          std::int64_t{47} /*addend*/);

    // Export the internal fixup array to the 'os' string-stream.
    pstore::exchange::export_ns::ostringstream os;
    emit_internal_fixups (os, pstore::exchange::export_ns::indent{}, std::begin (ifixups),
                          std::end (ifixups));

    // Setup the parse.
    pstore::exchange::import_ns::string_mapping imported_names;
    internal_fixup_collection imported_ifixups;
    auto parser = pstore::json::make_parser (
        pstore::exchange::import_ns::callbacks::make<internal_fixup_array_root> (
            &db_, &imported_names, &imported_ifixups));

    // Import the data that we just exported.
    parser.input (os.str ());
    parser.eof ();

    // Check that we got back what we started with.
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    EXPECT_THAT (imported_ifixups, testing::ContainerEq (ifixups))
        << "The imported and exported ifixups should match";
}

namespace {

    template <typename T>
    using not_null = pstore::gsl::not_null<T>;


    class InternalFixupMembersImport : public testing::Test {
    public:
        using transaction_lock = std::unique_lock<mock_mutex>;

        InternalFixupMembersImport ()
                : db_storage_{}
                , db_{db_storage_.file ()} {
            db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

        decltype (auto) parse (std::string const & src,
                               not_null<internal_fixup_collection *> const fixups) {
            using namespace pstore::exchange::import_ns;
            string_mapping names;
            auto parser =
                pstore::json::make_parser (callbacks::make<ifixups_object> (&db_, &names, fixups));
            parser.input (src).eof ();
            return parser;
        }

        decltype (auto) parse (std::string const & src) {
            internal_fixup_collection fixups;
            return parse (src, &fixups);
        }

    private:
        InMemoryStore db_storage_;

    protected:
        pstore::database db_;
    };


    // A test for all of the valid target secton names.
    using name_section_pair = std::tuple<pstore::gsl::czstring, pstore::repo::section_kind>;
    class InternalFixupSectionNames : public InternalFixupMembersImport,
                                      public testing::WithParamInterface<name_section_pair> {};

} // end anonymous namespace

TEST_P (InternalFixupSectionNames, SectionName) {
    auto const & ns = GetParam ();
    std::ostringstream os;
    os << R"({ "section" : ")" << std::get<pstore::gsl::czstring> (ns)
       << R"(", "type":17, "offset":19, "addend":-23 })";

    internal_fixup_collection fixups;
    auto const & parser = this->parse (os.str (), &fixups);
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();

    ASSERT_EQ (fixups.size (), 1U);
    EXPECT_EQ (fixups[0].section, std::get<pstore::repo::section_kind> (ns));
    EXPECT_EQ (fixups[0].type, 17U);
    EXPECT_EQ (fixups[0].offset, 19U);
    EXPECT_EQ (fixups[0].addend, -23);
}

#define X(x) name_section_pair{#x, pstore::repo::section_kind::x},
#ifdef PSTORE_IS_INSIDE_LLVM
INSTANTIATE_TEST_CASE_P (InternalFixupSectionNames, InternalFixupSectionNames,
                         testing::ValuesIn ({PSTORE_MCREPO_SECTION_KINDS}), );
#else
INSTANTIATE_TEST_SUITE_P (InternalFixupSectionNames, InternalFixupSectionNames,
                          testing::ValuesIn ({PSTORE_MCREPO_SECTION_KINDS}));
#endif
#undef X

TEST_F (InternalFixupMembersImport, SectionErrors) {
    // Section key is missing.
    {
        auto const & parser1 = this->parse (R"({ "type":17, "offset":19, "addend":23 })");
        ASSERT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (
            parser1.last_error (),
            make_error_code (pstore::exchange::import_ns::error::ifixup_object_was_incomplete));
    }
    // Section key has an unknown value.
    {
        auto const & parser2 =
            this->parse (R"({ "section":"bad", "type":17, "offset":19, "addend":23 })");
        EXPECT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_ns::error::unknown_section_name));
    }
    // Section key has the wrong type.
    {
        auto const & parser3 =
            this->parse (R"({ "section":false, "type":17, "offset":19, "addend":23 })");
        EXPECT_TRUE (parser3.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser3.last_error (),
                   make_error_code (pstore::exchange::import_ns::error::unexpected_boolean));
    }
}

TEST_F (InternalFixupMembersImport, TypeErrors) {
    // The type key is missing altogether.
    {
        auto const & parser1 = this->parse (R"({ "section":"text", "offset":19, "addend":23 })");
        EXPECT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (
            parser1.last_error (),
            make_error_code (pstore::exchange::import_ns::error::ifixup_object_was_incomplete));
    }
    // The type key has the wrong type.
    {
        auto const & parser2 =
            this->parse (R"({ "section":"text", "type":true, "offset":19, "addend":23 })");
        ASSERT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_ns::error::unexpected_boolean));
    }
}

TEST_F (InternalFixupMembersImport, OffsetErrors) {
    // The offset key is missing altogether.
    {
        auto const & parser1 = this->parse (R"({ "section":"text", "type":17, "addend":23 })");
        EXPECT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (
            parser1.last_error (),
            make_error_code (pstore::exchange::import_ns::error::ifixup_object_was_incomplete));
    }
    // The offset key has the wrong type.
    {
        auto const & parser2 =
            this->parse (R"({ "section":"text", "type":17, "offset":true, "addend":23 })");
        EXPECT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_ns::error::unexpected_boolean));
    }
    // Offset is negative.
    {
        auto const & parser3 =
            this->parse (R"({ "section":"text", "type":17, "offset":-3, "addend":23 })");
        EXPECT_TRUE (parser3.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser3.last_error (),
                   make_error_code (pstore::exchange::import_ns::error::unexpected_number));
    }
}

TEST_F (InternalFixupMembersImport, AddendErrors) {
    {
        auto const & parser1 = this->parse (R"({ "section":"text", "type":17, "offset":19 })");
        EXPECT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (
            parser1.last_error (),
            make_error_code (pstore::exchange::import_ns::error::ifixup_object_was_incomplete));
    }
    {
        auto const & parser2 =
            this->parse (R"({ "section":"text", "type":17, "offset":19, "addend":true })");
        EXPECT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_ns::error::unexpected_boolean));
    }
}

TEST_F (InternalFixupMembersImport, BadMember) {
    auto const & parser = this->parse (R"({ "bad":true })");
    EXPECT_TRUE (parser.has_error ()) << "Expected the parse to fail";
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_ns::error::unrecognized_ifixup_key));
}


namespace {

    using xfixup_collection = std::vector<pstore::repo::external_fixup>;
    using string_address = pstore::typed_address<pstore::indirect_string>;

    class ExchangeExternalFixups : public testing::Test {
    public:
        ExchangeExternalFixups ()
                : export_store_{}
                , export_db_{export_store_.file ()}
                , import_store_{}
                , import_db_{import_store_.file ()} {
            export_db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
            import_db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

        using transaction_lock = std::unique_lock<mock_mutex>;
        using xfixup_array_root =
            pstore::exchange::import_ns::array_rule<pstore::exchange::import_ns::xfixups_object,
                                                    pstore::exchange::import_ns::string_mapping *,
                                                    xfixup_collection *>;

        InMemoryStore export_store_;
        pstore::database export_db_;

        InMemoryStore import_store_;
        pstore::database import_db_;
    };

} // end anonymous namespace

TEST_F (ExchangeExternalFixups, ExternalEmpty) {
    using namespace pstore::exchange;
    constexpr auto comments = false;

    // Start with an empty collection of external fixups.
    xfixup_collection xfixups;

    // Export the internal fixup array to the 'os' string-stream.
    export_ns::ostringstream os;
    export_ns::string_mapping names{export_db_, export_ns::name_index_tag ()};
    emit_external_fixups (os, export_ns::indent{}, export_db_, names, std::begin (xfixups),
                          std::end (xfixups), comments);

    // Setup the parse.
    xfixup_collection imported_xfixups;
    import_ns::string_mapping imported_names;
    auto parser = pstore::json::make_parser (import_ns::callbacks::make<xfixup_array_root> (
        &import_db_, &imported_names, &imported_xfixups));

    // Import the data that we just exported.
    parser.input (os.str ());
    parser.eof ();

    // Check the result.
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    EXPECT_THAT (imported_xfixups, testing::ContainerEq (xfixups))
        << "The imported and exported xfixups should match";
}

TEST_F (ExchangeExternalFixups, RoundTripForTwoFixups) {
    using namespace pstore::exchange;
    constexpr auto comments = false;

    constexpr auto name_index = pstore::trailer::indices::name;
    std::vector<pstore::gsl::czstring> strings{"foo", "bar"};

    // Add these strings to the database.
    std::unordered_map<std::string, string_address> indir_strings;
    add_export_strings<name_index> (export_db_, std::begin (strings), std::end (strings),
                                    std::inserter (indir_strings, std::end (indir_strings)));



    // Write the names that we just created as JSON.
    export_ns::string_mapping exported_names{export_db_, export_ns::name_index_tag ()};
    export_ns::ostringstream exported_names_stream;
    export_ns::emit_strings<name_index> (exported_names_stream, export_ns::indent{}, export_db_,
                                         export_db_.get_current_revision (), "", &exported_names,
                                         comments);



    // Build a collection of external fixups. These refer to names added to the database
    // add_export_strings().
    xfixup_collection xfixups;
    xfixups.emplace_back (indir_strings["foo"], static_cast<pstore::repo::relocation_type> (5),
                          pstore::repo::reference_strength::strong, 7, 9);
    xfixups.emplace_back (indir_strings["bar"], static_cast<pstore::repo::relocation_type> (11),
                          pstore::repo::reference_strength::weak, 13, 17);



    // Export the external fixup array to the 'exported_fixups' string-stream.
    export_ns::ostringstream exported_fixups;
    emit_external_fixups (exported_fixups, export_ns::indent{}, export_db_, exported_names,
                          std::begin (xfixups), std::end (xfixups), comments);



    // Create matching names in the imported database.
    mock_mutex mutex;
    auto transaction = begin (import_db_, std::unique_lock<mock_mutex>{mutex});

    import_ns::string_mapping imported_names;
    {
        using namespace import_ns;
        auto parser = pstore::json::make_parser (
            callbacks::make<array_rule<strings_array_members, decltype (&transaction),
                                       decltype (&imported_names)>> (&import_db_, &transaction,
                                                                     &imported_names));
        parser.input (exported_names_stream.str ()).eof ();
        ASSERT_FALSE (parser.has_error ())
            << "Expected the JSON parse to succeed (" << parser.last_error ().message () << ')';
    }


    {
        xfixup_collection imported_xfixups;
        imported_xfixups.reserve (2);

        auto parser = pstore::json::make_parser (import_ns::callbacks::make<xfixup_array_root> (
            &import_db_, &imported_names, &imported_xfixups));
        parser.input (exported_fixups.str ()).eof ();

        // Check the result.
        ASSERT_FALSE (parser.has_error ())
            << "Expected the JSON parse to succeed (" << parser.last_error ().message () << ')';

        compare_external_fixups (export_db_, xfixups, import_db_, imported_xfixups);
    }
    transaction.commit ();
}


namespace {

    using external_fixup_collection = std::vector<pstore::repo::external_fixup>;

    class ExternalFixupMembersImport : public testing::Test {
    public:
        ExternalFixupMembersImport ()
                : store_{}
                , db_{store_.file ()} {
            db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

        using transaction_lock = std::unique_lock<mock_mutex>;

        static decltype (auto) parse (std::string const & src, pstore::database * const db,
                                      pstore::exchange::import_ns::string_mapping const & names,
                                      not_null<external_fixup_collection *> const fixups) {
            using namespace pstore::exchange::import_ns;
            auto parser =
                pstore::json::make_parser (callbacks::make<xfixups_object> (db, &names, fixups));
            parser.input (src);
            parser.eof ();
            return parser;
        }

        static decltype (auto) parse (std::string const & src, pstore::database * const db,
                                      pstore::exchange::import_ns::string_mapping const & names) {
            external_fixup_collection fixups;
            return parse (src, db, names, &fixups);
        }

        InMemoryStore store_;
        pstore::database db_;
    };

} // end anonymous namespace

TEST_F (ExternalFixupMembersImport, Name) {
    // Create matching names in the imported database.
    mock_mutex mutex;
    auto transaction = begin (db_, transaction_lock{mutex});

    pstore::exchange::import_ns::string_mapping imported_names;

    // The name key is missing altogether.
    {
        auto const & parser1 =
            this->parse (R"({ "type":13, "offset":19, "addend":23 })", &db_, imported_names);
        EXPECT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (
            parser1.last_error (),
            make_error_code (pstore::exchange::import_ns::error::xfixup_object_was_incomplete));
    }
    // The name key has the wrong type.
    {
        auto const & parser2 = this->parse (
            R"({ "name":"name", "type":13, "offset":19, "addend":23 })", &db_, imported_names);
        ASSERT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_ns::error::unexpected_string));
    }
    // The name key has a bad value.
    {
        auto const & parser3 = this->parse (R"({ "name":1, "type":13, "offset":19, "addend":23 })",
                                            &db_, imported_names);
        ASSERT_TRUE (parser3.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser3.last_error (),
                   make_error_code (pstore::exchange::import_ns::error::no_such_name));
    }
}

TEST_F (ExternalFixupMembersImport, Type) {
    // Create matching names in the imported database.
    mock_mutex mutex;
    auto transaction = begin (db_, transaction_lock{mutex});

    pstore::exchange::import_ns::string_mapping imported_names;
    // Add a single name with index 0.
    ASSERT_EQ (imported_names.add_string (&transaction, "name"), std::error_code{});

    // The type key is missing altogether.
    {
        auto const & parser1 =
            this->parse (R"({ "name":0, "offset":19, "addend":23 })", &db_, imported_names);
        EXPECT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (
            parser1.last_error (),
            make_error_code (pstore::exchange::import_ns::error::xfixup_object_was_incomplete));
    }
    // The type key has the wrong type.
    {
        auto const & parser2 = this->parse (
            R"({ "name":0, "type":true, "offset":19, "addend":23 })", &db_, imported_names);
        ASSERT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_ns::error::unexpected_boolean));
    }
}

TEST_F (ExternalFixupMembersImport, IsWeak) {
    mock_mutex mutex;
    auto transaction = begin (db_, transaction_lock{mutex});

    pstore::exchange::import_ns::string_mapping imported_names;
    // Add a single name with index 0.
    ASSERT_EQ (imported_names.add_string (&transaction, "name"), std::error_code{});

    // The is_weak key is missing altogether. That's okay: the default is 'false'.
    {
        auto const & parser1 = this->parse (R"({ "name":0, "type":17, "offset":19, "addend":23 })",
                                            &db_, imported_names);
        EXPECT_FALSE (parser1.has_error ())
            << "JSON error was: " << parser1.last_error ().message ();
    }
    // The is_weak key has the wrong type.
    {
        auto const & parser2 =
            this->parse (R"({ "name":0, "type":true, "is_weak":0, offset":19, "addend":23 })", &db_,
                         imported_names);
        ASSERT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_ns::error::unexpected_boolean));
    }
}

TEST_F (ExternalFixupMembersImport, Offset) {
    mock_mutex mutex;
    auto transaction = begin (db_, transaction_lock{mutex});
    pstore::exchange::import_ns::string_mapping imported_names;
    ASSERT_EQ (imported_names.add_string (&transaction, "name"), std::error_code{});

    // The offset key is missing altogether.
    {
        auto const & parser1 =
            this->parse (R"({ "name":0, "type":17, "addend":23 })", &db_, imported_names);
        EXPECT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (
            parser1.last_error (),
            make_error_code (pstore::exchange::import_ns::error::xfixup_object_was_incomplete));
    }
    // The offset key has the wrong type.
    {
        auto const & parser2 = this->parse (
            R"({ "name":0, "type":17, "offset":true, "addend":23 })", &db_, imported_names);
        EXPECT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_ns::error::unexpected_boolean));
    }
    // Offset is negative.
    {
        auto const & parser3 = this->parse (R"({ "name":0, "type":17, "offset":-3, "addend":23 })",
                                            &db_, imported_names);
        EXPECT_TRUE (parser3.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser3.last_error (),
                   make_error_code (pstore::exchange::import_ns::error::unexpected_number));
    }
}

TEST_F (ExternalFixupMembersImport, Addend) {
    mock_mutex mutex;
    auto transaction = begin (db_, transaction_lock{mutex});
    pstore::exchange::import_ns::string_mapping imported_names;
    ASSERT_EQ (imported_names.add_string (&transaction, "name"), std::error_code{});

    {
        auto const & parser1 =
            this->parse (R"({ "name":0, "type":17, "offset":19 })", &db_, imported_names);
        EXPECT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (
            parser1.last_error (),
            make_error_code (pstore::exchange::import_ns::error::xfixup_object_was_incomplete));
    }
    {
        auto const & parser2 = this->parse (
            R"({ "name":0, "type":17, "offset":19, "addend":true })", &db_, imported_names);
        EXPECT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_ns::error::unexpected_boolean));
    }
}

TEST_F (ExternalFixupMembersImport, BadMember) {
    pstore::exchange::import_ns::string_mapping imported_names;
    auto const & parser = this->parse (R"({ "bad":true })", &db_, imported_names);
    EXPECT_TRUE (parser.has_error ()) << "Expected the parse to fail";
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_ns::error::unrecognized_xfixup_key));
}
