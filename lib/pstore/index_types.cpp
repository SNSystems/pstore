//
//  index_types.cpp
//  gmock
//
//  Created by Paul Bowen-Huggett on 04/12/2017.
//

#include "pstore/index_types.hpp"

#include "pstore/transaction.hpp"
#include "pstore/hamt_map.hpp"
#include "pstore/hamt_set.hpp"

namespace {

    // get_index
    // ~~~~~~~~~
    template <typename IndexType>
    IndexType * get_index (pstore::database & db, enum pstore::trailer::indices which,
                           bool create) {
        auto & dx = db.get_index (which);

        // Have we already loaded this index?
        if (dx.get () == nullptr) {
            std::shared_ptr<pstore::trailer const> footer = db.get_footer ();
            pstore::address const location = footer->a.index_records.at (
                static_cast<std::underlying_type<decltype (which)>::type> (which));
            if (location == pstore::address::null ()) {
                if (create) {
                    // Create a new (empty) index.
                    dx = std::make_unique<IndexType> (db);
                }
            } else {
                // Construct the index from the location.
                dx = std::make_unique<IndexType> (db, location);
            }
        }

#ifdef PSTORE_CPP_RTTI
        assert ((!create && dx.get () == nullptr) ||
                dynamic_cast<IndexType *> (dx.get ()) != nullptr);
#endif
        return static_cast<IndexType *> (dx.get ());
    }
}

namespace pstore {
    namespace index {

        // get_write_index
        // ~~~~~~~~~~~~~~~
        write_index * get_write_index (database & db, bool create) {
            return get_index<write_index> (db, trailer::indices::write, create);
        }

        // get_digest_index
        // ~~~~~~~~~~~~~~~~
        digest_index * get_digest_index (database & db, bool create) {
            return get_index<digest_index> (db, trailer::indices::digest, create);
        }

        // get_ticket_index
        // ~~~~~~~~~~~~~~~~
        pstore::index::ticket_index * get_ticket_index (database & db, bool create) {
            return get_index<ticket_index> (db, trailer::indices::ticket, create);
        }

        // get_name_index
        // ~~~~~~~~~~~~~~
        name_index * get_name_index (database & db, bool create) {
            return get_index<name_index> (db, trailer::indices::name, create);
        }

        // flush_indices
        // ~~~~~~~~~~~~~
        void flush_indices (::pstore::transaction_base & transaction,
                            trailer::index_records_array * const locations) {
            pstore::database & db = transaction.db ();
            if (index::write_index * const write = get_write_index (db, false /*create*/)) {
                (*locations)[trailer::indices::write] = write->flush (transaction);
            }

            if (index::digest_index * const digest = get_digest_index (db, false /*create*/)) {
                (*locations)[trailer::indices::digest] = digest->flush (transaction);
            }

            if (index::ticket_index * const ticket = get_ticket_index (db, false /*create*/)) {
                (*locations)[trailer::indices::ticket] = ticket->flush (transaction);
            }

            if (index::name_index * const name = get_name_index (db, false /*create*/)) {
                (*locations)[trailer::indices::name] = name->flush (transaction);
            }

            assert (locations->size () == trailer::indices::last);
        }
    } // namespace index
} // namespace pstore
// eof index_types.cpp
