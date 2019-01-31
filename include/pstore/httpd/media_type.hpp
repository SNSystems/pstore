#ifndef PSTORE_HTTPD_MEDIA_TYPE_HPP
#define PSTORE_HTTPD_MEDIA_TYPE_HPP

#include <string>

#include "pstore/support/gsl.hpp"
#include "pstore/support/portab.hpp"

namespace pstore {
    namespace httpd {

        gsl::czstring PSTORE_NONNULL
        media_type_from_extension (gsl::czstring PSTORE_NONNULL extension);

        gsl::czstring PSTORE_NONNULL media_type_from_filename (std::string const & filename);

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTPD_MEDIA_TYPE_HPP
