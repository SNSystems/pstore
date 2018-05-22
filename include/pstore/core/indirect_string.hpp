//*  _           _ _               _         _        _              *
//* (_)_ __   __| (_)_ __ ___  ___| |_   ___| |_ _ __(_)_ __   __ _  *
//* | | '_ \ / _` | | '__/ _ \/ __| __| / __| __| '__| | '_ \ / _` | *
//* | | | | | (_| | | | |  __/ (__| |_  \__ \ |_| |  | | | | | (_| | *
//* |_|_| |_|\__,_|_|_|  \___|\___|\__| |___/\__|_|  |_|_| |_|\__, | *
//*                                                           |___/  *
//===- include/pstore/core/indirect_string.hpp ----------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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

#ifndef PSTORE_CORE_INDIRECT_STRING_HPP
#define PSTORE_CORE_INDIRECT_STRING_HPP (1)

#include <memory>

#include "pstore/core/db_archive.hpp"
#include "pstore/core/sstring_view_archive.hpp"

namespace pstore {

    using shared_sstring_view = sstring_view<std::shared_ptr<char const>>;

    //*  _         _ _            _        _       _            *
    //* (_)_ _  __| (_)_ _ ___ __| |_   __| |_ _ _(_)_ _  __ _  *
    //* | | ' \/ _` | | '_/ -_) _|  _| (_-<  _| '_| | ' \/ _` | *
    //* |_|_||_\__,_|_|_| \___\__|\__| /__/\__|_| |_|_||_\__, | *
    //*                                                  |___/  *
    /// The string address can come in three forms:
    /// 1. An shared_sstring_view string that hasn't been added to the index yet. This is indicated
    /// when is_pointer_ is true.
    /// 2. A database address which points to an in-memory shared_sstring_view. This happens when
    /// the string has been inserted, but the index has not yet been flushed. In this case,
    /// is_pointer_ is false and the LBS of address_ is set.
    /// 3. An address of a string in the store. In this case, is_pointer_ is false and the LSB of
    /// address_ is clear.
    ///
    /// The use of the LBS of the address field to distinguish between in-heap and in-store
    /// addresses means that the in-store string bodies must be 2-byte aligned.
    class indirect_string {
        friend struct serialize::serializer<indirect_string>;

    public:
        indirect_string (database & db, address addr)
                : db_{db}
                , is_pointer_{false}
                , address_{addr.absolute ()} {}
        indirect_string (database & db, shared_sstring_view const * str)
                : db_{db}
                , is_pointer_{true}
                , str_{str} {
            assert ((reinterpret_cast<std::uintptr_t> (str) & in_heap_mask) == 0);
        }

        bool operator== (indirect_string const & rhs) const;
        bool operator!= (indirect_string const & rhs) const { return !operator== (rhs); }
        bool operator< (indirect_string const & rhs) const;

        shared_sstring_view as_string_view () const;

        /// Write the body of a string and updates the indirect pointer so that it points to that
        /// body.
        ///
        /// \param transaction  The transaction to which the string body is appended.
        /// \param str  The string to be written.
        /// \param address_to_patch  The in-store address of the indirect_string instance which will
        /// point to the string.
        /// \returns  The address at which the string body was written.
        template <typename Transaction>
        static pstore::address write_body_and_patch_address (Transaction & transaction,
                                                             shared_sstring_view const & str,
                                                             address address_to_patch);

    private:
        static constexpr std::uint64_t in_heap_mask = 0x01;

        database & db_;
        // TODO: replace with std::variant<>...?
        /// True if the string address is in-heap and found in the str_ member. If false, the
        /// address_ field points to the string body in the store unless (address_ & in_heap_mask)
        /// in which case it is the heap address of the string.
        bool is_pointer_;
        union {
            std::uint64_t address_;           ///< The in-store/in-heap string address.
            shared_sstring_view const * str_; ///< The address of the in-heap string.
        };
    };

    std::ostream & operator<< (std::ostream & os, indirect_string const & ind_str);

    // write_string_and_patch_address
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    template <typename Transaction>
    address indirect_string::write_body_and_patch_address (Transaction & transaction,
                                                           shared_sstring_view const & str,
                                                           address address_to_patch) {
        assert (address_to_patch != address::null ());

        // Make sure the alignment of the string is 2 to ensure that the LSB is clear.
        constexpr auto aligned_to = std::size_t{1U << in_heap_mask};
        transaction.allocate (0, aligned_to);

        // Write the string body.
        auto const body_address =
            serialize::write (serialize::archive::make_writer (transaction), str);

        // Modify the in-store address field so that it points to the string body.
        auto addr = transaction.template getrw<address> (address_to_patch);
        *addr = body_address;
        return body_address;
    }


    namespace serialize {

        /// \brief A serializer for indirect_string.
        ///
        /// Note that this reads and writes an address: the body of the string must be read and
        /// written separately. For writing, see indirect_string::write_body_and_patch_address().
        template <>
        struct serializer<indirect_string> {
            using value_type = indirect_string;

            template <typename Transaction>
            static auto write (archive::database_writer<Transaction> && archive,
                               value_type const & value)
                -> archive_result_type<archive::database_writer<Transaction>>;

            /// \brief Reads an instance of `indirect_string` from an archiver.
            ///
            /// \param archive  The Archiver from which a string will be read.
            /// \param value  A reference to uninitialized memory that is suitable for a new string
            /// instance.
            /// \note This function only reads from the database.
            static void read (archive::database_reader & archive, value_type & value);
            static void read (archive::database_reader && archive, value_type & value);

        private:
            template <typename DBArchive>
            static void read_string_address (DBArchive && archive, value_type & value);
        };

        // write
        // ~~~~~
        template <typename Transaction>
        auto serializer<indirect_string>::write (archive::database_writer<Transaction> && archive,
                                                 value_type const & value)
            -> archive_result_type<archive::database_writer<Transaction>> {

            // The body of an indirect string must be written separately by the caller.
            assert (value.is_pointer_);
            assert (
                !(reinterpret_cast<std::uintptr_t> (value.str_) & indirect_string::in_heap_mask));
            return archive.put (address::make (reinterpret_cast<std::uintptr_t> (value.str_) |
                                               indirect_string::in_heap_mask));
        }

    } // end namespace serialize


    //*  _         _ _            _        _       _                     _    _          *
    //* (_)_ _  __| (_)_ _ ___ __| |_   __| |_ _ _(_)_ _  __ _   __ _ __| |__| |___ _ _  *
    //* | | ' \/ _` | | '_/ -_) _|  _| (_-<  _| '_| | ' \/ _` | / _` / _` / _` / -_) '_| *
    //* |_|_||_\__,_|_|_| \___\__|\__| /__/\__|_| |_|_||_\__, | \__,_\__,_\__,_\___|_|   *
    //*                                                  |___/                           *
    template <typename Transaction, typename Index,
              typename Container =
                  std::vector<std::pair<pstore::shared_sstring_view const *, pstore::address>>>
    class indirect_string_adder {
    public:
        indirect_string_adder (Transaction & transaction, std::shared_ptr<Index> const & name);
        indirect_string_adder (Transaction & transaction, std::shared_ptr<Index> const & name,
                               std::size_t expected_size);

        std::pair<typename Index::iterator, bool> add (pstore::shared_sstring_view const * str);
        void flush ();

    private:
        Transaction & transaction_;
        std::shared_ptr<Index> index_;
        Container views_;
    };

    // ctor
    // ~~~~
    template <typename Transaction, typename Index, typename Container>
    indirect_string_adder<Transaction, Index, Container>::indirect_string_adder (
        Transaction & transaction, std::shared_ptr<Index> const & name)
            : transaction_{transaction}
            , index_{name} {}

    template <typename Transaction, typename Index, typename Container>
    indirect_string_adder<Transaction, Index, Container>::indirect_string_adder (
        Transaction & transaction, std::shared_ptr<Index> const & name, std::size_t expected_size)
            : indirect_string_adder (transaction, name) {
        views_.reserve (expected_size);
    }

    // add
    // ~~~
    template <typename Transaction, typename Index, typename Container>
    std::pair<typename Index::iterator, bool>
    indirect_string_adder<Transaction, Index, Container>::add (
        pstore::shared_sstring_view const * str) {

        // Inserting into the index immediately writes the indirect_string instance to the store if
        // the string isn't already in the set.
        auto res = index_->insert (transaction_, pstore::indirect_string{transaction_.db (), str});
        if (res.second) {
            // Now the in-store addresses are pointing at the sstring_view instances on the heap.
            // If the string was written, we remember where it went.
            typename Index::iterator const & pos = res.first;
            views_.emplace_back (str, pos.get_address ());
        }
        return res;
    }

    // flush
    // ~~~~~
    template <typename Transaction, typename Index, typename Container>
    void indirect_string_adder<Transaction, Index, Container>::flush () {
        for (auto const & v : views_) {
            assert (v.second != pstore::address::null ());
            indirect_string::write_body_and_patch_address (transaction_,
                                                           *std::get<0> (v), // string body
                                                           std::get<1> (v)   // address to patch
            );
        }
        views_.clear ();
    }

    // make_indirect_string_adder
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~
    template <typename Transaction, typename Index,
              typename Container =
                  std::vector<std::pair<pstore::shared_sstring_view const *, pstore::address>>>
    auto make_indirect_string_adder (Transaction & transaction,
                                     std::shared_ptr<Index> const & index)
        -> indirect_string_adder<Transaction, Index, Container> {

        return {transaction, index};
    }

} // end namespace pstore

#endif // PSTORE_CORE_INDIRECT_STRING_HPP
// eof: include/pstore/core/indirect_string.hpp
