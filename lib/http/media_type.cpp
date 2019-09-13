//*                     _ _         _                     *
//*  _ __ ___   ___  __| (_) __ _  | |_ _   _ _ __   ___  *
//* | '_ ` _ \ / _ \/ _` | |/ _` | | __| | | | '_ \ / _ \ *
//* | | | | | |  __/ (_| | | (_| | | |_| |_| | |_) |  __/ *
//* |_| |_| |_|\___|\__,_|_|\__,_|  \__|\__, | .__/ \___| *
//*                                     |___/|_|          *
//===- lib/http/media_type.cpp --------------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
        media_type_from_extension (gsl::czstring PSTORE_NONNULL extension) {
            static auto const begin = std::begin (media_types);
            static auto const end = std::end (media_types);

            assert (std::is_sorted (
                begin, end,
                [](media_type_entry const & lhs, media_type_entry const & rhs) noexcept {
                    return std::strcmp (lhs.name, rhs.name) < 0;
                }));

            auto const pos = std::lower_bound (
                begin, end, extension, [](media_type_entry const & mte, char const * ext) noexcept {
                    return std::strcmp (mte.name, ext) < 0;
                });
            return pos != end && std::strcmp (extension, pos->name) == 0
                       ? pos->media_type
                       : "application/octet-stream";
        }

        gsl::czstring PSTORE_NONNULL media_type_from_filename (std::string const & filename) {
            auto const dot_pos = filename.rfind ('.');
            return media_type_from_extension (
                filename.c_str () + (dot_pos != std::string::npos ? dot_pos : filename.length ()));
        }

    } // end namespace httpd
} // end namespace pstore
