//*                     _ _         _                     *
//*  _ __ ___   ___  __| (_) __ _  | |_ _   _ _ __   ___  *
//* | '_ ` _ \ / _ \/ _` | |/ _` | | __| | | | '_ \ / _ \ *
//* | | | | | |  __/ (_| | | (_| | | |_| |_| | |_) |  __/ *
//* |_| |_| |_|\___|\__,_|_|\__,_|  \__|\__, | .__/ \___| *
//*                                     |___/|_|          *
//===- lib/http/media_type.cpp --------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/http/media_type.hpp"

#include <algorithm>
#include <cstring>

#include "pstore/support/portab.hpp"

namespace {

    struct media_type_entry {
        char const * PSTORE_NONNULL name;
        char const * PSTORE_NONNULL media_type;
    };

    constexpr media_type_entry media_types[] = {
        {".css", "text/css"},
        {".gif", "image/gif"},
        {".htm", "text/html"},
        {".html", "text/html"},
        {".jpeg", "image/jpeg"},
        {".jpg", "image/jpeg"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".svg", "image/svg+xml"},
        {".txt", "text/plain"},
        {".xml", "application/xml"},
    };

} // end anonymous namespace

namespace pstore {
    namespace httpd {

        gsl::czstring PSTORE_NONNULL
        media_type_from_extension (gsl::czstring const PSTORE_NONNULL extension) {
            static auto const begin = std::begin (media_types);
            static auto const end = std::end (media_types);

            PSTORE_ASSERT (std::is_sorted (
                begin, end,
                [] (media_type_entry const & lhs, media_type_entry const & rhs) noexcept {
                    return std::strcmp (lhs.name, rhs.name) < 0;
                }));

            auto const pos = std::lower_bound (
                begin, end, extension,
                [] (media_type_entry const & mte, gsl::czstring const ext) noexcept {
                    return std::strcmp (mte.name, ext) < 0;
                });
            return pos != end && std::strcmp (extension, pos->name) == 0
                       ? pos->media_type
                       : "application/octet-stream";
        }

        gsl::czstring PSTORE_NONNULL media_type_from_filename (std::string const & filename) {
            auto const dot_pos = filename.rfind ('.');
            return media_type_from_extension (
                filename.c_str () + (dot_pos == std::string::npos ? filename.length () : dot_pos));
        }

    } // end namespace httpd
} // end namespace pstore
