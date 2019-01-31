#ifndef PSTORE_HTTPD_QUERY_TO_KVP_HPP
#define PSTORE_HTTPD_QUERY_TO_KVP_HPP

#include <unordered_map>
#include <string>

#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace httpd {

        using query_kvp = std::unordered_map<std::string, std::string>;

        query_kvp query_to_kvp (std::string const & in);
        query_kvp query_to_kvp (gsl::czstring in);

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTPD_QUERY_TO_KVP_HPP
