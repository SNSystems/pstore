//*                                _        _   _       *
//*  ___  ___ _ ____   _____   ___| |_ __ _| |_(_) ___  *
//* / __|/ _ \ '__\ \ / / _ \ / __| __/ _` | __| |/ __| *
//* \__ \  __/ |   \ V /  __/ \__ \ || (_| | |_| | (__  *
//* |___/\___|_|    \_/ \___| |___/\__\__,_|\__|_|\___| *
//*                                                     *
//*                  _             _    *
//*   ___ ___  _ __ | |_ ___ _ __ | |_  *
//*  / __/ _ \| '_ \| __/ _ \ '_ \| __| *
//* | (_| (_) | | | | ||  __/ | | | |_  *
//*  \___\___/|_| |_|\__\___|_| |_|\__| *
//*                                     *
//===- unittests/http/test_serve_static_content.cpp -----------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/http/serve_static_content.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <string>

#include "pstore/http/request.hpp"
#include "pstore/http/send.hpp"
#include "pstore/support/array_elements.hpp"
#include "pstore/support/maybe.hpp"

#include <gmock/gmock.h>

namespace {

    char const index_html[] = "<!DOCTYPE html><html></html>";
    static constexpr std::size_t index_size = pstore::array_elements (index_html) - 1U;

    extern pstore::romfs::directory const root_dir;
    constexpr std::time_t index_mtime = 1556010627;
    std::array<pstore::romfs::dirent, 3> const root_dir_membs = {{
        {".", &root_dir},
        {"..", &root_dir},
        {"index.html", reinterpret_cast<std::uint8_t const *> (index_html),
         pstore::romfs::stat{index_size, index_mtime, pstore::romfs::mode_t::file}},
    }};
    pstore::romfs::directory const root_dir{root_dir_membs};
    pstore::romfs::directory const * const root = &root_dir;

} // end anonymous namespace

namespace {

    class ServeStaticContent : public ::testing::Test {
    public:
        ServeStaticContent ()
                : fs_{root} {}

    protected:
        pstore::romfs::romfs const & fs () const noexcept { return fs_; }
        pstore::error_or<std::string> serve_path (std::string const & path) const;

    private:
        pstore::romfs::romfs fs_;
    };

    pstore::error_or<std::string> ServeStaticContent::serve_path (std::string const & path) const {
        std::string actual;

        using eoint = pstore::error_or<int>;
        auto sender = [&actual] (int io, pstore::gsl::span<std::uint8_t const> const & sp) {
            std::transform (std::begin (sp), std::end (sp), std::back_inserter (actual),
                            [] (std::uint8_t b) { return static_cast<char> (b); });
            return eoint (io + 1);
        };

        return pstore::http::serve_static_content (sender, 0, path, fs ()) >>= [&actual] (int) {
            return pstore::error_or<std::string>{pstore::in_place, actual};
        };
    }



    class reader {
    public:
        using state_type = std::string::size_type;

        explicit reader (std::string const & src) noexcept
                : src_{src} {}

        decltype (auto) gets (state_type const start) {
            using result_type = pstore::error_or_n<state_type, pstore::maybe<std::string>>;

            static auto const crlf = pstore::http::crlf;
            static auto const crlf_len = std::strlen (crlf);

            PSTORE_ASSERT (start != std::string::npos);
            auto const pos = src_.find (crlf, start);
            PSTORE_ASSERT (pos != std::string::npos);
            return result_type{pstore::in_place, pos + crlf_len,
                               pstore::just (src_.substr (start, pos - start))};
        }

    private:
        std::string const & src_;
    };

} // end anonymous namespace


TEST_F (ServeStaticContent, Simple) {
    pstore::error_or<std::string> const actual = serve_path ("/index.html");
    ASSERT_TRUE (static_cast<bool> (actual));

    using string_pair = std::pair<std::string, std::string>;
    std::vector<string_pair> headers;

    reader r{*actual};
    auto const record_headers = [&r, &headers] (reader::state_type io,
                                                pstore::http::request_info const &) {
        auto record_header = [&headers] (int io2, std::string const & key,
                                         std::string const & value) {
            // The date value will change according to when the test is run so we preserve its
            // presence but drop its value.
            headers.emplace_back (key, (key == "date") ? "" : value);
            return io2 + 1;
        };

        return pstore::http::read_headers (r, io, record_header, 0);
    };

    pstore::error_or_n<std::string::size_type, int> const eo =
        pstore::http::read_request (r, std::string::size_type{0}) >>= record_headers;

    ASSERT_TRUE (static_cast<bool> (eo));
    EXPECT_THAT (headers,
                 ::testing::UnorderedElementsAre (
                     string_pair{"content-length", "28"}, string_pair{"content-type", "text/html"},
                     string_pair{"date", ""}, string_pair{"connection", "close"},
                     string_pair{"last-modified", "Tue, 23 Apr 2019 09:10:27 GMT"},
                     string_pair{"server", "pstore-http"}));
    EXPECT_EQ ((std::string{*actual, std::get<0> (eo)}), index_html);
}

TEST_F (ServeStaticContent, MissingFile) {
    pstore::error_or<std::string> const actual = serve_path ("/foo.html");
    EXPECT_EQ (actual.get_error (), make_error_code (pstore::romfs::error_code::enoent));
}
