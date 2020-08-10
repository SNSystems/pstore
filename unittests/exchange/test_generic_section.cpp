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

#include "pstore/exchange/import_section_to_importer.hpp"
#include "pstore/json/json.hpp"

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

    template <pstore::repo::section_kind Kind>
    decltype (auto) build_section (pstore::gsl::not_null<std::vector<std::uint8_t> *> const buffer,
                                   pstore::repo::section_content const & content) {
        // The type used to store the properties of section_kind Kind section.
        using section_type = typename pstore::repo::enum_to_section<Kind>::type;

        auto dispatcher = std::make_unique<
            typename pstore::repo::section_to_creation_dispatcher<section_type>::type> (Kind,
                                                                                        &content);
        buffer->resize (dispatcher->size_bytes ());
        dispatcher->write (buffer->data ());
        return reinterpret_cast<section_type const *> (buffer->data ());
    }

} // end anonymous namespace

TEST_F (GenericSection, RoundTripForAnEmptySection) {

    constexpr auto kind = pstore::repo::section_kind::text;
    // The type used to store a text section's properties.
    using section_type = pstore::repo::enum_to_section<kind>::type;
    static_assert (std::is_same<section_type, pstore::repo::generic_section>::value,
                   "Expected text to map to generic_section");

    pstore::repo::section_content text_content;
    std::vector<std::uint8_t> export_section_buffer;
    auto const * const section = build_section<kind> (&export_section_buffer, text_content);


    std::ostringstream os;
    pstore::exchange::export_name_mapping exported_names;
    pstore::exchange::export_section<kind> (os, export_db_, exported_names, *section);



    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    auto oit = std::back_inserter (dispatchers);

    pstore::exchange::import_name_mapping imported_names;
    pstore::repo::section_content imported_content;

    // Find the rule that is used to import sections represented by an instance of section_type.
    using section_importer = pstore::exchange::section_to_importer_t<section_type, decltype (oit)>;
    // A rule which will match an object wrapping the section_importer keys.
    using section_importer_object_root =
        pstore::exchange::object_rule<section_importer, decltype (kind),
                                      decltype (std::ref (import_db_)), decltype (&imported_names),
                                      decltype (&imported_content), decltype (&oit)>;

    auto parser =
        pstore::json::make_parser (pstore::exchange::callbacks::make<section_importer_object_root> (
            pstore::repo::section_kind::text, import_db_, &imported_names, &imported_content,
            &oit));
    parser.input (os.str ()).eof ();
    EXPECT_FALSE (parser.has_error ());
    EXPECT_EQ (parser.last_error (), std::error_code{});
}
