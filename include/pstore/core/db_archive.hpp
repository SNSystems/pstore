//*      _ _                      _     _            *
//*   __| | |__     __ _ _ __ ___| |__ (_)_   _____  *
//*  / _` | '_ \   / _` | '__/ __| '_ \| \ \ / / _ \ *
//* | (_| | |_) | | (_| | | | (__| | | | |\ V /  __/ *
//*  \__,_|_.__/   \__,_|_|  \___|_| |_|_| \_/ \___| *
//*                                                  *
//===- include/pstore/core/db_archive.hpp ---------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
/// \file db_archive.hpp
/// \brief Provides the database_reader and database_writer class which enable
/// the serializer to read and write types in a pstore instance.

#ifndef PSTORE_CORE_DB_ARCHIVE_HPP
#define PSTORE_CORE_DB_ARCHIVE_HPP

#include "pstore/core/address.hpp"
#include "pstore/core/database.hpp"
#include "pstore/serialize/archive.hpp"
#include "pstore/support/error.hpp"

namespace pstore {
    namespace serialize {
        /// \brief Contains the serialization archive types.
        namespace archive {

            // *************************************
            // *   d a t a b a s e _ w r i t e r   *
            // *************************************
            namespace details {

                template <typename Transaction>
                class database_writer_policy {
                public:
                    using result_type = pstore::address;

                    explicit database_writer_policy (Transaction & trans) noexcept
                            : transaction_ (trans) {}

                    /// Writes an instance of a standard-layout type T to the database.
                    /// \param value  The value to be written to the output container.
                    /// \returns The pstore address at which the value was written.
                    template <typename Ty>
                    auto put (Ty const & value) -> result_type {
                        std::shared_ptr<Ty> ptr;
                        auto addr = typed_address<Ty>::null ();
                        std::tie (ptr, addr) = transaction_.template alloc_rw<Ty> ();
                        *ptr = value;
                        return addr.to_address ();
                    }

                    template <typename Span>
                    auto putn (Span sp) -> result_type {
                        using element_type =
                            typename std::remove_const<typename Span::element_type>::type;

                        std::shared_ptr<element_type> ptr;
                        auto addr = typed_address<element_type>::null ();
                        std::tie (ptr, addr) = transaction_.template alloc_rw<element_type> (
                            unsigned_cast (sp.size ()));
                        std::copy (std::begin (sp), std::end (sp), ptr.get ());
                        return addr.to_address ();
                    }

                    void flush () noexcept {}

                private:
                    /// The transaction to which data is written.
                    Transaction & transaction_;
                };

            } // namespace details

            template <typename Transaction>
            class database_writer final
                    : public writer_base<details::database_writer_policy<Transaction>> {
                using policy = details::database_writer_policy<Transaction>;
            public:
                /// \brief Constructs the writer using the transaction.
                /// \param transaction The active transaction to the store to which the
                ///                    database_writer will write.
                explicit database_writer (Transaction & transaction)
                        : writer_base<policy> (policy {transaction}) {}
            };

            /// A convenience function which simplifies the construction of a database_writer
            /// instance if the caller has an existing transaction object.
            /// \param transaction The transaction to which the database_writer will append.
            template <typename Transaction>
            inline auto make_writer (Transaction & transaction) noexcept
                -> database_writer<Transaction> {
                return database_writer<Transaction>{transaction};
            }


            // *************************************
            // *   d a t a b a s e _ r e a d e r   *
            // *************************************

            /// \brief An archive-reader which reads data from a database.
            class database_reader {
            public:
                /// Constructs the reader using an input database and an address.
                ///
                /// \param db The database from which data is read.
                /// \param addr The start address from which data is read.
                database_reader (pstore::database const & db, pstore::address const addr) noexcept
                        : db_ (db)
                        , addr_ (addr) {}

                pstore::database const & get_db () const noexcept { return db_; }
                pstore::address get_address () const noexcept { return addr_; }
                void skip (std::size_t const distance) noexcept { addr_ += distance; }

                /// Reads a single instance of a standard-layout type Ty from the current store
                /// address.
                ///
                /// \param v  Uninitialized memory into which the new instance of Ty should be
                /// placed.
                ///
                /// \tparam Ty  A standard-layout type.
                template <typename Ty, typename = typename std::enable_if<
                                           std::is_standard_layout<Ty>::value>::type>
                void get (Ty & v);

                /// Reads a span of a trivial type from the current store address.
                ///
                /// \param span  A span of uninitialized memory into which the data will be placed.
                ///
                /// \tparam SpanType  A GSL span which describes a range of uninitialized memory.
                template <typename SpanType,
                          typename = typename std::enable_if<std::is_standard_layout<
                              typename SpanType::element_type>::value>::type>
                void getn (SpanType span);

            private:
                database const & db_; ///< The database from which data is read.
                address addr_;        ///< The address from which data is read.
            };

            // get
            // ~~~
            template <typename Ty, typename>
            void database_reader::get (Ty & v) {

                auto const extra_for_alignment = calc_alignment (addr_.absolute (), alignof (Ty));
                assert (extra_for_alignment < sizeof (Ty));
                addr_ += extra_for_alignment;
                // Load the data.
                auto result = db_.getro (typed_address<Ty> (addr_));
                addr_ += sizeof (Ty);
                // Copy to the destination.
                new (&v) Ty (*result);
            }

            // getn
            // ~~~~
            template <typename SpanType, typename>
            void database_reader::getn (SpanType span) {
                using element_type = typename SpanType::element_type;

                // Adjust addr_ so that it is correctly aligned for element_type.
                auto const extra_for_alignment =
                    calc_alignment (addr_.absolute (), alignof (element_type));
                assert (extra_for_alignment < sizeof (element_type));
                addr_ += extra_for_alignment;

                // Load the data.
                auto const size = unsigned_cast (span.size_bytes ());
                auto src = db_.getro (typed_address<std::uint8_t> (addr_), size);
                addr_ += size;

                // Copy to the destination span.
                auto first = src.get ();
                std::copy (first, first + size, reinterpret_cast<std::uint8_t *> (span.data ()));
            }

            /// A convenience function which provides symmetry with the make_writer() function.
            /// Constructs a database reader using an input database and an address.
            ///
            /// \param db  The database from which data will be read.
            /// \param addr  The address at which to start reading.
            /// \result A database reader instance which will read the given database at the
            /// specified address.
            inline database_reader make_reader (pstore::database const & db,
                                                pstore::address const addr) noexcept {
                return {db, addr};
            }
        } // namespace archive
    }     // namespace serialize
} // namespace pstore
#endif // PSTORE_CORE_DB_ARCHIVE_HPP
