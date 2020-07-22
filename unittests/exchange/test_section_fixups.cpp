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
#include "export_fixups.hpp"
#include "import_fixups.hpp"

#include <sstream>
#include <vector>

#include "pstore/json/json.hpp"
#include "pstore/exchange/import_rule.hpp"
#include "pstore/exchange/import_non_terminals.hpp"

#include <gmock/gmock.h>

#include "empty_store.hpp"

namespace {

    using ifixup_collection = std::vector<pstore::repo::internal_fixup>;
    using ifixup_array_root =
        pstore::exchange::array_rule<pstore::exchange::ifixups_object, pstore::exchange::names *,
                                     ifixup_collection *>;

} // end anonymous namespace

TEST (ExchangeSectionFixups, InternalEmpty) {
    // Start with an empty collection of internal fixups.
    ifixup_collection ifixups;

    // Export the internal fixup array to the 'os' string-stream.
    std::ostringstream os;
    pstore::exchange::emit_section_ifixups (os, std::begin (ifixups), std::end (ifixups));

    // Setup the parse.
    pstore::exchange::names names;
    ifixup_collection imported_ifixups;
    auto parser = pstore::json::make_parser (
        pstore::exchange::callbacks::make<ifixup_array_root> (&names, &imported_ifixups));

    // Import the data that we just exported.
    parser.input (os.str ());
    parser.eof ();

    // Check the result.
    EXPECT_FALSE (parser.has_error ()) << "Expected the JSON parse to succeed";
    EXPECT_THAT (imported_ifixups, testing::ContainerEq (ifixups))
        << "The imported and exported ifixups should match";
}

TEST (ExchangeSectionFixups, InternalCollection) {
    // Start with a small collection of internal fixups.
    ifixup_collection ifixups;
    ifixups.emplace_back (pstore::repo::section_kind::text, pstore::repo::relocation_type{17},
                          std::uint64_t{19} /*offset */, std::uint64_t{23} /*addend*/);
    ifixups.emplace_back (pstore::repo::section_kind::text, pstore::repo::relocation_type{29},
                          std::uint64_t{31} /*offset */, std::uint64_t{37} /*addend*/);
    ifixups.emplace_back (pstore::repo::section_kind::thread_data,
                          pstore::repo::relocation_type{41}, std::uint64_t{43} /*offset */,
                          std::uint64_t{47} /*addend*/);

    // Export the internal fixup array to the 'os' string-stream.
    std::ostringstream os;
    pstore::exchange::emit_section_ifixups (os, std::begin (ifixups), std::end (ifixups));

    // Setup the parse.
    pstore::exchange::names names;
    ifixup_collection imported_ifixups;
    auto parser = pstore::json::make_parser (
        pstore::exchange::callbacks::make<ifixup_array_root> (&names, &imported_ifixups));

    // Import the data that we just exported.
    parser.input (os.str ());
    parser.eof ();

    // Check that we got back what we started with.
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    EXPECT_THAT (imported_ifixups, testing::ContainerEq (ifixups))
        << "The imported and exported ifixups should match";
}

namespace {

    using xfixup_collection = std::vector<pstore::repo::external_fixup>;
    using xfixup_array_root =
        pstore::exchange::array_rule<pstore::exchange::xfixups_object, pstore::exchange::names *,
                                     xfixup_collection *>;

    class ExchangeExternalFixups : public EmptyStore {
    public:
        ExchangeExternalFixups ()
                : db_{this->file ()} {
            db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

        pstore::database & db () { return db_; }

    private:
        pstore::database db_;
    };

} // end anonymous namespace

TEST_F (ExchangeExternalFixups, ExternalEmpty) {
    // Start with an empty collection of external fixups.
    xfixup_collection xfixups;

    // Export the internal fixup array to the 'os' string-stream.
    std::ostringstream os;
    pstore::exchange::name_mapping names;
    pstore::exchange::emit_section_xfixups (os, this->db (), names, std::begin (xfixups),
                                            std::end (xfixups));

    // Setup the parse.
    xfixup_collection imported_xfixups;
    pstore::exchange::names imported_names;
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
