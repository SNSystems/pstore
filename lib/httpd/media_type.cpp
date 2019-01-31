#include "pstore/httpd/media_type.hpp"

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
                begin,
                end, [](media_type_entry const & lhs, media_type_entry const & rhs) noexcept {
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
