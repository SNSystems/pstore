#ifndef PSTORE_EXCHANGE_EXPORT_PATHS_HPP
#define PSTORE_EXCHANGE_EXPORT_PATHS_HPP

#include "pstore/exchange/export_names.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            void emit_paths (ostream_base & os, indent const ind, database const & db,
                             unsigned const generation,
                             gsl::not_null<name_mapping *> const string_table);

        } // end namespace export_ns
    }     // end namespace exchange
} // namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_PATHS_HPP
