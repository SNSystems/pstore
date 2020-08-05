//*                _   _                __ _                       *
//*  ___  ___  ___| |_(_) ___  _ __    / _(_)_  ___   _ _ __  ___  *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  | |_| \ \/ / | | | '_ \/ __| *
//* \__ \  __/ (__| |_| | (_) | | | | |  _| |>  <| |_| | |_) \__ \ *
//* |___/\___|\___|\__|_|\___/|_| |_| |_| |_/_/\_\\__,_| .__/|___/ *
//*                                                    |_|         *
//===- unittests/exchange/test_section_fixups.cpp -------------------------===//
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
#include "pstore/exchange/export_fixups.hpp"
#include "pstore/exchange/import_fixups.hpp"

#include <sstream>
#include <vector>

#include "pstore/adt/sstring_view.hpp"
#include "pstore/json/json.hpp"
#include "pstore/exchange/import_error.hpp"
#include "pstore/exchange/import_rule.hpp"
#include "pstore/exchange/import_non_terminals.hpp"
#include "pstore/exchange/import_names_array.hpp"

#include <gmock/gmock.h>

#include "empty_store.hpp"

namespace {

    using transaction_lock = std::unique_lock<mock_mutex>;

    using internal_fixup_collection = std::vector<pstore::repo::internal_fixup>;
    using internal_fixup_array_root =
        pstore::exchange::array_rule<pstore::exchange::ifixups_object<transaction_lock>,
                                     pstore::exchange::import_name_mapping *,
                                     internal_fixup_collection *>;

} // end anonymous namespace

TEST (ExchangeSectionFixups, RoundTripInternalEmpty) {
    // Start with an empty collection of internal fixups.
    internal_fixup_collection ifixups;

    // Export the internal fixup array to the 'os' string-stream.
    std::ostringstream os;
    pstore::exchange::export_internal_fixups (os, std::begin (ifixups), std::end (ifixups));

    // Setup the parse.
    pstore::exchange::import_name_mapping names;
    internal_fixup_collection imported_ifixups;
    auto parser = pstore::json::make_parser (
        pstore::exchange::callbacks::make<internal_fixup_array_root> (&names, &imported_ifixups));

    // Import the data that we just exported.
    parser.input (os.str ());
    parser.eof ();

    // Check the result.
    EXPECT_FALSE (parser.has_error ()) << "Expected the JSON parse to succeed";
    EXPECT_THAT (imported_ifixups, testing::ContainerEq (ifixups))
        << "The imported and exported ifixups should match";
}

TEST (ExchangeSectionFixups, RoundTripInternalCollection) {
    // Start with a small collection of internal fixups.
    internal_fixup_collection ifixups;
    ifixups.emplace_back (pstore::repo::section_kind::text, pstore::repo::relocation_type{17},
                          std::uint64_t{19} /*offset */, std::uint64_t{23} /*addend*/);
    ifixups.emplace_back (pstore::repo::section_kind::text, pstore::repo::relocation_type{29},
                          std::uint64_t{31} /*offset */, std::uint64_t{37} /*addend*/);
    ifixups.emplace_back (pstore::repo::section_kind::thread_data,
                          pstore::repo::relocation_type{41}, std::uint64_t{43} /*offset */,
                          std::uint64_t{47} /*addend*/);

    // Export the internal fixup array to the 'os' string-stream.
    std::ostringstream os;
    pstore::exchange::export_internal_fixups (os, std::begin (ifixups), std::end (ifixups));

    // Setup the parse.
    pstore::exchange::import_name_mapping imported_names;
    internal_fixup_collection imported_ifixups;
    auto parser =
        pstore::json::make_parser (pstore::exchange::callbacks::make<internal_fixup_array_root> (
            &imported_names, &imported_ifixups));

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

        static decltype (auto) parse (std::string const & src,
                                      not_null<internal_fixup_collection *> const fixups) {
            pstore::exchange::import_name_mapping names;
            auto parser = pstore::json::make_parser (
                pstore::exchange::callbacks::make<
                    pstore::exchange::ifixups_object<transaction_lock>> (&names, fixups));
            parser.input (src);
            parser.eof ();
            return parser;
        }

        static decltype (auto) parse (std::string const & src) {
            internal_fixup_collection fixups;
            return parse (src, &fixups);
        }
    };


    // A test for all of the valid target secton names.
    using name_section_pair = std::tuple<pstore::gsl::czstring, pstore::repo::section_kind>;
    class InternalFixupSectionNames : public InternalFixupMembersImport,
                                      public testing::WithParamInterface<name_section_pair> {};

} // end anonymous namespace

TEST_P (InternalFixupSectionNames, SectionName) {
    auto const & ns = GetParam ();
    std::ostringstream os;
    os << R"({ "section" : ")" << std::get<pstore::gsl::czstring> (ns) << R"(", )";
    os << R"("type":17, "offset":19, "addend":23 })";

    internal_fixup_collection fixups;
    auto const & parser = this->parse (os.str (), &fixups);
    ASSERT_FALSE (parser.has_error ()) << "Expected the parse to succeed";

    ASSERT_EQ (fixups.size (), 1U);
    EXPECT_EQ (fixups[0].section, std::get<pstore::repo::section_kind> (ns));
    EXPECT_EQ (fixups[0].type, 17U);
    EXPECT_EQ (fixups[0].offset, 19U);
    EXPECT_EQ (fixups[0].addend, 23U);
}

#define X(x) name_section_pair{#x, pstore::repo::section_kind::x},
#ifdef PSTORE_IS_INSIDE_LLVM
INSTANTIATE_TEST_CASE_P (IFixupSectionNames, IFixupSectionNames,
                         testing::ValuesIn ({PSTORE_MCREPO_SECTION_KINDS}));
#else
INSTANTIATE_TEST_SUITE_P (IFixupSectionNames, InternalFixupSectionNames,
                          testing::ValuesIn ({PSTORE_MCREPO_SECTION_KINDS}));
#endif
#undef X

TEST_F (InternalFixupMembersImport, MissingSection) {
    // Section key is missing.
    {
        auto const & parser1 = this->parse (R"({ "type":17, "offset":19, "addend":23 })");
        ASSERT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser1.last_error (),
                   make_error_code (pstore::exchange::import_error::ifixup_object_was_incomplete));
    }
    // Section key has an unknown value.
    {
        auto const & parser2 =
            this->parse (R"({ "section":"bad", "type":17, "offset":19, "addend":23 })");
        EXPECT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_error::unknown_section_name));
    }
    // Section key has the wrong type.
    {
        auto const & parser3 =
            this->parse (R"({ "section":false, "type":17, "offset":19, "addend":23 })");
        EXPECT_TRUE (parser3.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser3.last_error (),
                   make_error_code (pstore::exchange::import_error::unexpected_boolean));
    }
}

TEST_F (InternalFixupMembersImport, Type) {
    // The type key is missing altogether.
    {
        auto const & parser1 = this->parse (R"({ "section":"text", "offset":19, "addend":23 })");
        EXPECT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser1.last_error (),
                   make_error_code (pstore::exchange::import_error::ifixup_object_was_incomplete));
    }
    // The type key has the wrong type.
    {
        auto const & parser2 =
            this->parse (R"({ "section":"text", "type":true, "offset":19, "addend":23 })");
        ASSERT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_error::unexpected_boolean));
    }
}

TEST_F (InternalFixupMembersImport, Offset) {
    // The offset key is missing altogether.
    {
        auto const & parser1 = this->parse (R"({ "section":"text", "type":17, "addend":23 })");
        EXPECT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser1.last_error (),
                   make_error_code (pstore::exchange::import_error::ifixup_object_was_incomplete));
    }
    // The offset key has the wrong type.
    {
        auto const & parser2 =
            this->parse (R"({ "section":"text", "type":17, "offset":true, "addend":23 })");
        EXPECT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_error::unexpected_boolean));
    }
    // Offset is negative.
    {
        auto const & parser3 =
            this->parse (R"({ "section":"text", "type":17, "offset":-3, "addend":23 })");
        EXPECT_TRUE (parser3.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser3.last_error (),
                   make_error_code (pstore::exchange::import_error::unexpected_number));
    }
}

TEST_F (InternalFixupMembersImport, Addend) {
    {
        auto const & parser1 = this->parse (R"({ "section":"text", "type":17, "offset":19 })");
        EXPECT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser1.last_error (),
                   make_error_code (pstore::exchange::import_error::ifixup_object_was_incomplete));
    }
    {
        auto const & parser2 =
            this->parse (R"({ "section":"text", "type":17, "offset":19, "addend":true })");
        EXPECT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_error::unexpected_boolean));
    }
}

TEST_F (InternalFixupMembersImport, BadMember) {
    auto const & parser = this->parse (R"({ "bad":true })");
    EXPECT_TRUE (parser.has_error ()) << "Expected the parse to fail";
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_error::unrecognized_ifixup_key));
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
            pstore::exchange::array_rule<pstore::exchange::xfixups_object<transaction_lock>,
                                         pstore::exchange::import_name_mapping *,
                                         xfixup_collection *>;

        InMemoryStore export_store_;
        pstore::database export_db_;

        InMemoryStore import_store_;
        pstore::database import_db_;
    };

} // end anonymous namespace

TEST_F (ExchangeExternalFixups, ExternalEmpty) {
    // Start with an empty collection of external fixups.
    xfixup_collection xfixups;

    // Export the internal fixup array to the 'os' string-stream.
    std::ostringstream os;
    pstore::exchange::export_name_mapping names;
    pstore::exchange::export_external_fixups (os, export_db_, names, std::begin (xfixups),
                                              std::end (xfixups));

    // Setup the parse.
    xfixup_collection imported_xfixups;
    pstore::exchange::import_name_mapping imported_names;
    auto parser = pstore::json::make_parser (
        pstore::exchange::callbacks::make<xfixup_array_root> (&imported_names, &imported_xfixups));

    // Import the data that we just exported.
    parser.input (os.str ());
    parser.eof ();

    // Check the result.
    EXPECT_FALSE (parser.has_error ()) << "Expected the JSON parse to succeed";
    EXPECT_THAT (imported_xfixups, testing::ContainerEq (xfixups))
        << "The imported and exported xfixups should match";
}

namespace {

    pstore::raw_sstring_view load_string (pstore::database const & db, string_address addr,
                                          not_null<pstore::shared_sstring_view *> owner) {
        using namespace pstore::serialize;
        return read<pstore::indirect_string> (archive::database_reader{db, addr.to_address ()})
            .as_string_view (owner);
    }

    std::string load_std_string (pstore::database const & db, string_address addr) {
        pstore::shared_sstring_view owner;
        return std::string{load_string (db, addr, &owner)};
    }


    template <typename InputIterator, typename OutputIterator>
    void add_export_strings (pstore::database & db, InputIterator first, InputIterator last,
                             OutputIterator out) {
        mock_mutex mutex;
        auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});
        auto const name_index = pstore::index::get_index<pstore::trailer::indices::name> (db);

        std::vector<pstore::raw_sstring_view> views;
        std::transform (first, last, std::back_inserter (views),
                        [] (pstore::gsl::czstring str) { return pstore::make_sstring_view (str); });

        pstore::indirect_string_adder adder;
        std::transform (
            std::begin (views), std::end (views), out, [&] (pstore::raw_sstring_view const & view) {
                std::pair<pstore::index::name_index::iterator, bool> const res1 =
                    adder.add (transaction, name_index, &view);
                return std::make_pair (view.to_string (),
                                       string_address::make (res1.first.get_address ()));
            });

        adder.flush (transaction);
        transaction.commit ();
    }

    void compare_external_fixups (pstore::database const & export_db,
                                  xfixup_collection & exported_xfixups,
                                  pstore::database const & import_db,
                                  xfixup_collection & imported_xfixups) {
        ASSERT_EQ (imported_xfixups.size (), exported_xfixups.size ())
            << "Expected the number of xfixups imported to match the number we started with";

        // The name fields are tricky here. The imported and exported fixups are from different
        // databases so we can't simply compare string addresses to find out if they point to the
        // same string. Instead we must load each of the strings and compare them directly.
        // However, we still want to use operator== for all of the other fields so that we don't
        // end up having to duplicate the rest of the comparison method here. Setting both name
        // fields to 0 after comparison allows us to do that.
        {
            auto export_it = std::begin (exported_xfixups);
            auto import_it = std::begin (imported_xfixups);
            auto import_end = std::end (imported_xfixups);
            auto count = std::size_t{0};
            for (; import_it != import_end; ++import_it, ++export_it, ++count) {
                EXPECT_EQ (load_std_string (import_db, import_it->name),
                           load_std_string (export_db, export_it->name))
                    << "Names of fixup #" << count;

                // Set the import and export name values to the same address (it doesn't matter what
                // that address is). This will mean that differences won't cause failures as we
                // compare the two containers.
                export_it->name = import_it->name = string_address ();
            }
        }

        EXPECT_THAT (imported_xfixups, testing::ContainerEq (exported_xfixups))
            << "The imported and exported xfixups should match";
    }


} // end anonymous namespace

TEST_F (ExchangeExternalFixups, RoundTripForTwoFixups) {
    std::vector<pstore::gsl::czstring> strings{"foo", "bar"};

    // Add these strings to the database.
    std::unordered_map<std::string, string_address> indir_strings;
    add_export_strings (export_db_, std::begin (strings), std::end (strings),
                        std::inserter (indir_strings, std::end (indir_strings)));



    // Write the names that we just created as JSON.
    pstore::exchange::export_name_mapping exported_names;
    std::ostringstream exported_names_stream;
    export_names (exported_names_stream, export_db_, export_db_.get_current_revision (),
                  &exported_names);



    // Build a collection of external fixups. These refer to names added to the database
    // add_export_strings().
    xfixup_collection xfixups;
    xfixups.emplace_back (indir_strings["foo"], static_cast<pstore::repo::relocation_type> (5), 7,
                          9);
    xfixups.emplace_back (indir_strings["bar"], static_cast<pstore::repo::relocation_type> (11), 13,
                          17);



    // Export the external fixup array to the 'exported_fixups' string-stream.
    std::ostringstream exported_fixups;
    pstore::exchange::export_external_fixups (exported_fixups, export_db_, exported_names,
                                              std::begin (xfixups), std::end (xfixups));



    // Create matching names in the imported database.
    mock_mutex mutex;
    auto transaction = begin (import_db_, std::unique_lock<mock_mutex>{mutex});

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


    {
        xfixup_collection imported_xfixups;
        imported_xfixups.reserve (2);

        auto parser =
            pstore::json::make_parser (pstore::exchange::callbacks::make<xfixup_array_root> (
                &imported_names, &imported_xfixups));
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

        static decltype (auto) parse (std::string const & src,
                                      pstore::exchange::import_name_mapping const & names,
                                      not_null<external_fixup_collection *> const fixups) {
            auto parser = pstore::json::make_parser (
                pstore::exchange::callbacks::make<
                    pstore::exchange::xfixups_object<transaction_lock>> (&names, fixups));
            parser.input (src);
            parser.eof ();
            return parser;
        }

        static decltype (auto) parse (std::string const & src,
                                      pstore::exchange::import_name_mapping const & names) {
            external_fixup_collection fixups;
            return parse (src, names, &fixups);
        }

        InMemoryStore store_;
        pstore::database db_;
    };

} // end anonymous namespace

TEST_F (ExternalFixupMembersImport, Name) {
    // Create matching names in the imported database.
    mock_mutex mutex;
    auto transaction = begin (db_, transaction_lock{mutex});

    pstore::exchange::import_name_mapping imported_names;

    // The name key is missing altogether.
    {
        auto const & parser1 =
            this->parse (R"({ "type":13, "offset":19, "addend":23 })", imported_names);
        EXPECT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser1.last_error (),
                   make_error_code (pstore::exchange::import_error::xfixup_object_was_incomplete));
    }
    // The name key has the wrong type.
    {
        auto const & parser2 = this->parse (
            R"({ "name":"name", "type":13, "offset":19, "addend":23 })", imported_names);
        ASSERT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_error::unexpected_string));
    }
    // The name key has a bad value.
    {
        auto const & parser3 =
            this->parse (R"({ "name":1, "type":13, "offset":19, "addend":23 })", imported_names);
        ASSERT_TRUE (parser3.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser3.last_error (),
                   make_error_code (pstore::exchange::import_error::no_such_name));
    }
}

TEST_F (ExternalFixupMembersImport, Type) {
    // Create matching names in the imported database.
    mock_mutex mutex;
    auto transaction = begin (db_, transaction_lock{mutex});

    pstore::exchange::import_name_mapping imported_names;
    // Add a single name with index 0.
    ASSERT_EQ (imported_names.add_string (&transaction, "name"), std::error_code{});

    // The type key is missing altogether.
    {
        auto const & parser1 =
            this->parse (R"({ "name":0, "offset":19, "addend":23 })", imported_names);
        EXPECT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser1.last_error (),
                   make_error_code (pstore::exchange::import_error::xfixup_object_was_incomplete));
    }
    // The type key has the wrong type.
    {
        auto const & parser2 =
            this->parse (R"({ "name":0, "type":true, "offset":19, "addend":23 })", imported_names);
        ASSERT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_error::unexpected_boolean));
    }
}

TEST_F (ExternalFixupMembersImport, Offset) {
    mock_mutex mutex;
    auto transaction = begin (db_, transaction_lock{mutex});
    pstore::exchange::import_name_mapping imported_names;
    ASSERT_EQ (imported_names.add_string (&transaction, "name"), std::error_code{});

    // The offset key is missing altogether.
    {
        auto const & parser1 =
            this->parse (R"({ "name":0, "type":17, "addend":23 })", imported_names);
        EXPECT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser1.last_error (),
                   make_error_code (pstore::exchange::import_error::xfixup_object_was_incomplete));
    }
    // The offset key has the wrong type.
    {
        auto const & parser2 =
            this->parse (R"({ "name":0, "type":17, "offset":true, "addend":23 })", imported_names);
        EXPECT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_error::unexpected_boolean));
    }
    // Offset is negative.
    {
        auto const & parser3 =
            this->parse (R"({ "name":0, "type":17, "offset":-3, "addend":23 })", imported_names);
        EXPECT_TRUE (parser3.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser3.last_error (),
                   make_error_code (pstore::exchange::import_error::unexpected_number));
    }
}

TEST_F (ExternalFixupMembersImport, Addend) {
    mock_mutex mutex;
    auto transaction = begin (db_, transaction_lock{mutex});
    pstore::exchange::import_name_mapping imported_names;
    ASSERT_EQ (imported_names.add_string (&transaction, "name"), std::error_code{});

    {
        auto const & parser1 =
            this->parse (R"({ "name":0, "type":17, "offset":19 })", imported_names);
        EXPECT_TRUE (parser1.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser1.last_error (),
                   make_error_code (pstore::exchange::import_error::xfixup_object_was_incomplete));
    }
    {
        auto const & parser2 =
            this->parse (R"({ "name":0, "type":17, "offset":19, "addend":true })", imported_names);
        EXPECT_TRUE (parser2.has_error ()) << "Expected the parse to fail";
        EXPECT_EQ (parser2.last_error (),
                   make_error_code (pstore::exchange::import_error::unexpected_boolean));
    }
}

TEST_F (ExternalFixupMembersImport, BadMember) {
    pstore::exchange::import_name_mapping imported_names;
    auto const & parser = this->parse (R"({ "bad":true })", imported_names);
    EXPECT_TRUE (parser.has_error ()) << "Expected the parse to fail";
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_error::unrecognized_xfixup_key));
}
