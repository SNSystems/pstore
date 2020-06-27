#ifndef PSTORE_EXCHANGE_EXPORT_HPP
#define PSTORE_EXCHANGE_EXPORT_HPP

#include "pstore/core/database.hpp"
#include "pstore/exchange/export_ostream.hpp"

namespace pstore {
    namespace exchange {

        void export_database (database & db, crude_ostream & os);

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_HPP
