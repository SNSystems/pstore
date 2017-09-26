//*   __ _ _        _                    _            *
//*  / _(_) | ___  | |__   ___  __ _  __| | ___ _ __  *
//* | |_| | |/ _ \ | '_ \ / _ \/ _` |/ _` |/ _ \ '__| *
//* |  _| | |  __/ | | | |  __/ (_| | (_| |  __/ |    *
//* |_| |_|_|\___| |_| |_|\___|\__,_|\__,_|\___|_|    *
//*                                                   *
//===- include/pstore/file_header.hpp -------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc. 
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
/// \file file_header.hpp
/// \brief The file header and footer member functions
///
/// The capacity of an individual segment is defined by offset_number_bits (i.e. the largest
/// offset that we can encode before we need to start again with a new segment). This is
/// 4Mb, which is considerably smaller than I'd like because of the fact that the Windows
/// virtual memory system resizes the underlying file to match to ....
///
///
/// The initial state of the file is shown below. The file simply contains its header structure
/// an an inital (empty) transaction (_t_<sub>0</sub>).
///
/// \image html store_file_format_t0.svg
///
/// The header and footer types are pstore::header and pstore::trailer repectively.
/// The state of the file after the first transaction (_t_<sub>1</sub>) has been committed:
///
/// \image html store_file_format_t1.svg
///
/// A thread connecting to the data store uses the pstore::header::footer_pos value to find the most
/// recent completed transaction; this is an instance of pstore::trailer and marks the _end_ of the
/// data associated with that transaction.

#ifndef PSTORE_FILE_HEADER_HPP
#define PSTORE_FILE_HEADER_HPP (1)

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <ostream>

//#include "pstore_support/file.hpp"
#include "pstore/address.hpp"
#include "pstore/serialize/types.hpp"
#include "pstore/uuid.hpp"

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(x) static_assert (x, #x)
#endif

namespace pstore {

    /// This type is used to represent a BLOB of data: be it either an index
    /// key or an associated value.
    struct record {
        constexpr record () noexcept {}
        constexpr record (address addr_, std::uint64_t size_) noexcept : addr (addr_),
                                                                         size (size_) {}
        record (record const & rhs) noexcept = default;
        record & operator= (record const &) noexcept = default;

        /// The address of the data associated with this record.
        address addr = address::null ();
        /// The size of the data associated with this record.
        std::uint64_t size = UINT64_C (0);
    };

    STATIC_ASSERT (offsetof (record, addr) == 0);
    STATIC_ASSERT (offsetof (record, size) == 8);
    STATIC_ASSERT (sizeof (record) == 16);

    inline std::ostream & operator<< (std::ostream & os, record const & r) {
        return os << "{addr:" << r.addr << ",size:" << r.size << "}";
    }

    namespace serialize {
        /// \brief A specialization which teaches the serialization framework how to read and write instances of
        /// `record`.
        template <>
        struct serializer<record> {
            using value_type = record;

            template <typename Archive>
            static auto write (Archive & archive, value_type const & r) -> typename Archive::result_type {
                auto resl = serialize::write (archive, r.addr.absolute ());
                serialize::write (archive, r.size);
                return resl;
            }
            template <typename Archive>
            static void read (Archive & archive, value_type & r) {
                auto addr = address::make (serialize::read<std::uint64_t> (archive));
                auto size = serialize::read <std::uint64_t> (archive);
                new (&r) record (addr, size);
            }
        };
    } // namespace serialize


    /// \brief The data store file header.
    class header {
    public:
        header ();

        bool is_valid () const noexcept;

        /// Computes the CRC value for the header.
        std::uint32_t get_crc () const noexcept;

        static std::uint16_t const major_version = 0;
        static std::uint16_t const minor_version = 1;

        static std::array<std::uint8_t, 4> const file_signature1;
        static std::uint32_t const file_signature2 = 0x0507FFFF;

        /// Represents the portion of the header structure which is covered by the computed CRC
        /// value.
        struct body {
            /// The file signature is split into two pieces of four bytes each.
            /// The first of these (signature1) is an array of bytes so that the
            /// signature is easily recongisable in a hex dump, the second is a
            /// 32-bit value so that we can easily verify the machine endianness
            /// (a BOM in effect).

            std::array<std::uint8_t, 4> signature1;

            /// The second half of the file signature. This value is used to determine the
            /// endian-ness
            /// of the file.
            std::uint32_t signature2;
            std::array<std::uint16_t, 2> version;
            std::uint32_t header_size = sizeof (header);
            class uuid uuid;
        } a;

        /// The fields of the header, up to and including this one, are not modified as the
        /// code interacts with the data store; they're effectively read-only. Unfortunately, we
        /// can't make them physically read-only -- for example by marking the containing memory
        /// page as read-only -- because the library does need to be able to modify the
        /// #footer_pos field when a transaction is committed.
        ///
        /// This crc is used to ensure that the fields from #signature1 to #sync_name are not
        /// modified.
        std::uint32_t crc = 0;

        std::uint64_t unused2 = 0;

        /// The file offset of the current (most recent) file footer. This value is modified as the
        /// the very last step of commiting a transaction.
        std::atomic<address> footer_pos;
    };

    STATIC_ASSERT (offsetof (header::body, signature1) == 0);
    STATIC_ASSERT (offsetof (header::body, signature2) == 4);
    STATIC_ASSERT (offsetof (header::body, version) == 8);
    STATIC_ASSERT (offsetof (header::body, header_size) == 12);
    STATIC_ASSERT (offsetof (header::body, uuid) == 16);
    STATIC_ASSERT (sizeof (header::body) == 32);

    STATIC_ASSERT (offsetof (header, a) == 0);
    STATIC_ASSERT (offsetof (header, crc) == 32);
    STATIC_ASSERT (offsetof (header, unused2) == 40);
    STATIC_ASSERT (offsetof (header, footer_pos) == 48);
    STATIC_ASSERT (alignof (header) == 8);
    STATIC_ASSERT (sizeof (header) == 56);


    class database;

    /// \brief The transaction footer structure.
    /// An copy of this structure is written to the data store at the end of each transaction block.
    /// pstore::header::footer_pos holds the address of the latest _complete_ instance and is
    /// updated
    /// once a transaction has been completely written to memory. Once written it is read-only.
    struct trailer {
        static std::array<std::uint8_t, 8> default_signature1;
        static std::array<std::uint8_t, 8> default_signature2;

        bool crc_is_valid () const;

        /// Checks that the address given by 'pos' appears to point to a valid transaction trailer.
        /// Raises exception::footer_corrupt if not.
        static bool validate (database const & db, address pos);

        /// Computes the trailer's CRC value.
        std::uint32_t get_crc () const noexcept;

        enum indices {
            write,
            digest,
			ticket,
            name,
            last,
        };

        using index_records_array = std::array<address, indices::last>;

        /// Represents the portion of the trailer structure which is covered by the computed CRC
        /// value.
        struct body {
            std::array<std::uint8_t, 8> signature1 = default_signature1;
            std::atomic<std::uint32_t> generation{0};
            std::uint32_t unused1{0};

            /// The number of bytes contained by this transaction. The value does not include the
            /// size
            /// of the footer record.
            std::atomic<std::uint64_t> size{0};

            /// The time at which the transaction was committed, in milliseconds since the epoch.
            std::atomic<std::uint64_t> time{0};

            /// A pointer to the previous generation. This field forms a reverse linked list which
            /// allows a consumer to enumerate the generations contained within the store and to
            /// "sync" to a specific number.
            address prev_generation = address::null ();

            index_records_array index_records;
            std::uint32_t unused2{0};
            std::uint32_t unused3{0};
        };


        body a;

        /// The fields of a transaction footer are not modified as the code interacts
        /// with the data store. The memory that is occupies as marked as read-only as soon as
        /// the host OS and hardware permits. Despite this guarantee it's useful to be able
        /// to ensure that the reverse-order linked list of transactions -- whose head is given
        /// by header::footer_pos is intact and that we don't have a stray pointer.
        std::uint32_t crc = 0;
        std::uint32_t unused1 = 0;
        std::array<std::uint8_t, 8> signature2 = default_signature2;
    };

    STATIC_ASSERT (offsetof (trailer::body, signature1) == 0);
    STATIC_ASSERT (offsetof (trailer::body, generation) == 8);
    STATIC_ASSERT (offsetof (trailer::body, unused1) == 12);
    STATIC_ASSERT (offsetof (trailer::body, size) == 16);
    STATIC_ASSERT (offsetof (trailer::body, time) == 24);
    STATIC_ASSERT (offsetof (trailer::body, prev_generation) == 32);
    STATIC_ASSERT (offsetof (trailer::body, index_records) == 40);
    STATIC_ASSERT (offsetof (trailer::body, unused2) == 72);
    STATIC_ASSERT (offsetof (trailer::body, unused3) == 76);
    STATIC_ASSERT (sizeof (trailer::body) == 80);

    STATIC_ASSERT (offsetof (trailer, a) == 0);
    STATIC_ASSERT (offsetof (trailer, crc) == 80);
    STATIC_ASSERT (offsetof (trailer, signature2) == 88);
    STATIC_ASSERT (alignof (trailer) == 8);
    STATIC_ASSERT (sizeof (trailer) == 96);
} // namespace pstore
#endif // PSTORE_FILE_HEADER_HPP
// eof: include/pstore/file_header.hpp
