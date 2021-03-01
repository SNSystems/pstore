//===- unittests/exchange/compare_external_fixups.hpp -----*- mode: C++ -*-===//
//*                                            *
//*   ___ ___  _ __ ___  _ __   __ _ _ __ ___  *
//*  / __/ _ \| '_ ` _ \| '_ \ / _` | '__/ _ \ *
//* | (_| (_) | | | | | | |_) | (_| | | |  __/ *
//*  \___\___/|_| |_| |_| .__/ \__,_|_|  \___| *
//*                     |_|                    *
//*            _                        _    __ _                       *
//*   _____  _| |_ ___ _ __ _ __   __ _| |  / _(_)_  ___   _ _ __  ___  *
//*  / _ \ \/ / __/ _ \ '__| '_ \ / _` | | | |_| \ \/ / | | | '_ \/ __| *
//* |  __/>  <| ||  __/ |  | | | | (_| | | |  _| |>  <| |_| | |_) \__ \ *
//*  \___/_/\_\\__\___|_|  |_| |_|\__,_|_| |_| |_/_/\_\\__,_| .__/|___/ *
//*                                                         |_|         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_UNITTESTS_EXCHANGE_COMPARE_EXTERNAL_FIXUPS_HPP
#define PSTORE_UNITTESTS_EXCHANGE_COMPARE_EXTERNAL_FIXUPS_HPP

#include "pstore/adt/sstring_view.hpp"

template <typename ExportsContainer, typename ImportsContainer>
void compare_external_fixups (pstore::database const & export_db,
                              ExportsContainer & exported_fixups,
                              pstore::database const & import_db,
                              ImportsContainer & imported_fixups) {

    ASSERT_EQ (exported_fixups.size (), imported_fixups.size ())
        << "Expected the number of xfixups imported to match the number we started with";

    using string_address = pstore::typed_address<pstore::indirect_string>;

    auto const load_string = [] (pstore::database const & db, string_address const addr,
                                 pstore::gsl::not_null<pstore::shared_sstring_view *> const owner) {
        using namespace pstore::serialize;
        return read<pstore::indirect_string> (archive::database_reader{db, addr.to_address ()})
            .as_string_view (owner);
    };

    // The name fields are tricky here. The imported and exported fixups are from different
    // databases so we can't simply compare string addresses to find out if they point to the
    // same string. Instead we must load each of the strings and compare them directly.
    // However, we still want to use operator== for all of the other fields so that we don't
    // end up having to duplicate the rest of the comparison method here. Setting both name
    // fields to 0 after comparison allows us to do that.
    {
        auto import_it = std::begin (imported_fixups);
        auto export_it = std::begin (exported_fixups);
        auto const export_end = std::end (exported_fixups);
        auto count = std::size_t{0};
        for (; export_it != export_end; ++import_it, ++export_it, ++count) {
            pstore::shared_sstring_view import_owner;
            pstore::shared_sstring_view export_owner;
            EXPECT_EQ (load_string (import_db, import_it->name, &import_owner),
                       load_string (export_db, export_it->name, &export_owner))
                << "Names of fixup #" << count << ". exported name:" << export_it->name
                << ", imported name:" << import_it->name;

            // Set the import and export name values to the same address (it doesn't matter what
            // that address is). This will mean that differences won't cause failures as we
            // compare the two containers.
            export_it->name = import_it->name = string_address ();
        }
    }

    EXPECT_THAT (imported_fixups, testing::ContainerEq (exported_fixups))
        << "The imported and exported xfixups should match";
}

#endif // PSTORE_UNITTESTS_EXCHANGE_COMPARE_EXTERNAL_FIXUPS_HPP
