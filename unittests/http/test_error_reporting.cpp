#include "pstore/http/error_reporting.hpp"

#include <vector>

#include <gtest/gtest.h>

TEST (StatusLine, SwitchingProtocols) {
    EXPECT_EQ (pstore::http::build_status_line (pstore::http::http_status_code::switching_protocols,
                                                "path"),
               "HTTP/1.1 101 path\r\n");
}

TEST (StatusLine, BadRequest) {
    EXPECT_EQ (
        pstore::http::build_status_line (pstore::http::http_status_code::bad_request, "path"),
        "HTTP/1.1 400 path\r\n");
}

TEST (StatusLine, NotFound) {
    EXPECT_EQ (pstore::http::build_status_line (pstore::http::http_status_code::not_found, "path"),
               "HTTP/1.1 404 path\r\n");
}

TEST (StatusLine, UpgradeRequired) {
    EXPECT_EQ (
        pstore::http::build_status_line (pstore::http::http_status_code::upgrade_required, "path"),
        "HTTP/1.1 426 path\r\n");
}

TEST (StatusLine, InternalServerError) {
    EXPECT_EQ (pstore::http::build_status_line (
                   pstore::http::http_status_code::internal_server_error, "path"),
               "HTTP/1.1 500 path\r\n");
}

TEST (StatusLine, NotImplemented) {
    EXPECT_EQ (
        pstore::http::build_status_line (pstore::http::http_status_code::not_implemented, "path"),
        "HTTP/1.1 501 path\r\n");
}

TEST (BuildHeaders, Empty) {
    std::vector<pstore::http::czstring_pair> const h;
    auto const expected = "Server: pstore-http\r\n\r\n";
    EXPECT_EQ (pstore::http::build_headers (std::begin (h), std::end (h)), expected);
}

TEST (BuildHeaders, TwoSimple) {
    std::vector<pstore::http::czstring_pair> const h{{
        {"Content-length", "13"},
        {"Content-type", "text/html"},
    }};
    auto const expected = "Content-length: 13\r\n"
                          "Content-type: text/html\r\n"
                          "Server: pstore-http\r\n\r\n";
    // Send the three parts: the response line, the headers, and the HTML content.
    EXPECT_EQ (pstore::http::build_headers (std::begin (h), std::end (h)), expected);
}

TEST (SendErrorPage, A) {
    using testing::IsSubstring;

    std::string acc;
    auto const sender = [&acc] (int io, pstore::gsl::span<std::uint8_t const> const & s) {
        std::transform (std::begin (s), std::end (s), std::back_inserter (acc),
                        [] (std::uint8_t c) { return static_cast<char> (c); });
        return pstore::error_or<int>{io};
    };

    pstore::http::send_error_page (sender, 0, "cause", pstore::http::http_status_code::not_found,
                                   "short message", "this is a long message");
    EXPECT_PRED_FORMAT2 (IsSubstring, "HTTP/1.1 404 OK\r\n", acc);
    EXPECT_PRED_FORMAT2 (IsSubstring, "<p>404: short message</p>", acc);
    EXPECT_PRED_FORMAT2 (IsSubstring, "<p>this is a long message: cause</p>", acc);
}
