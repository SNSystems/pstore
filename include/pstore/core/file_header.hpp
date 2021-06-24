//===- include/pstore/core/file_header.hpp ----------------*- mode: C++ -*-===//
//*   __ _ _        _                    _            *
//*  / _(_) | ___  | |__   ___  __ _  __| | ___ _ __  *
//* | |_| | |/ _ \ | '_ \ / _ \/ _` |/ _` |/ _ \ '__| *
//* |  _| | |  __/ | | | |  __/ (_| | (_| |  __/ |    *
//* |_| |_|_|\___| |_| |_|\___|\__,_|\__,_|\___|_|    *
//*                                                   *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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

#ifndef PSTORE_CORE_FILE_HEADER_HPP
#define PSTORE_CORE_FILE_HEADER_HPP

#include <atomic>

#include "pstore/core/address.hpp"
#include "pstore/core/uuid.hpp"
#include "pstore/serialize/types.hpp"

namespace pstore {

    namespace serialize {
        /// \brief A specialization which teaches the serialization framework how to read and write
        /// instances of `extent`.
        template <typename T>
        struct serializer<extent<T>> {
            using value_type = extent<T>;

            template <typename Archive>
            static auto write (Archive && archive, value_type const & r)
                -> archive_result_type<Archive> {
                auto const resl =
                    serialize::write (std::forward<Archive> (archive), r.addr.absolute ());
                serialize::write (std::forward<Archive> (archive), r.size);
                return resl;
            }
            template <typename Archive>
            static void read (Archive && archive, value_type & r) {
                auto const addr = typed_address<T>::make (
                    serialize::read<std::uint64_t> (std::forward<Archive> (archive)));
                auto const size = serialize::read<std::uint64_t> (std::forward<Archive> (archive));
                new (&r) extent<T> (addr, size);
            }
        };
    } // namespace serialize

    struct trailer;

    /// \brief The data store file header.
    class header {
    public:
        header ();

        bool is_valid () const noexcept;

        /// Computes the CRC value for the header.
        std::uint32_t get_crc () const noexcept;

        /// Returns the database ID. When created, each pstore file has a unique ID number. It is
        /// preserved by import/export and strip/merge. External references may use this ID to
        /// check that they are referring to the correct database.
        uuid id () const noexcept { return a.id; }
        void set_id (uuid const & id) noexcept;

        /// Returns the file format version number (major, minor).
        std::array<std::uint16_t, 2> const & version () const noexcept { return a.version; }

        static constexpr std::uint16_t major_version = 1;
        static constexpr std::uint16_t minor_version = 12;

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
            /// endian-ness of the file.
            std::uint32_t signature2;

            /// The file format version number (major, minor).
            std::array<std::uint16_t, 2> version;
            std::uint32_t header_size = sizeof (header);
            /// The database ID.
            class uuid id;
        };

        body a;

        /// The fields of the header, up to and including this one, are not modified as the
        /// code interacts with the data store; they're effectively read-only. Unfortunately, we
        /// can't make them physically read-only -- for example by marking the containing memory
        /// page as read-only -- because the library does need to be able to modify the
        /// #footer_pos field when a transaction is committed.
        ///
        /// This crc is used to ensure that the fields from #signature1 to #sync_name are not
        /// modified.
        std::uint32_t crc = 0;
        std::uint32_t unused1 = 0;

        /// The file offset of the current (most recent) file footer. This value is modified as the
        /// the very last step of commiting a transaction.
        std::atomic<typed_address<trailer>> footer_pos;
    };

    // Assert the size, offset, and alignment of the structure and its fields to ensure file format
    // compatibility across compilers and hosts.
    PSTORE_STATIC_ASSERT (offsetof (header::body, signature1) == 0);
    PSTORE_STATIC_ASSERT (offsetof (header::body, signature2) == 4);
    PSTORE_STATIC_ASSERT (offsetof (header::body, version) == 8);
    PSTORE_STATIC_ASSERT (offsetof (header::body, header_size) == 12);
    PSTORE_STATIC_ASSERT (offsetof (header::body, id) == 16);
    PSTORE_STATIC_ASSERT (sizeof (header::body) == 32);

    PSTORE_STATIC_ASSERT (offsetof (header, a) == 0);
    PSTORE_STATIC_ASSERT (offsetof (header, crc) == 32);
    PSTORE_STATIC_ASSERT (offsetof (header, footer_pos) == 40);
    PSTORE_STATIC_ASSERT (alignof (header) == 8);
    PSTORE_STATIC_ASSERT (sizeof (header) == 48);

    /// \brief The lock-block is a small struct placed immediately after the file header which is
    /// used by the transaction lock.
    ///
    /// This data is not read or written but a file range lock is placed on it as part of the
    /// implementation of the transaction lock.
    struct lock_block {
        std::uint64_t vacuum_lock{chars_to_uint64 ('V', 'a', 'c', 'u', 'u', 'm', 'L', 'k')};
        std::uint64_t transaction_lock{chars_to_uint64 ('T', 'r', 'n', 's', 'a', 'c', 't', 'L')};

        static constexpr auto file_offset = std::uint64_t{sizeof (header)};
        static constexpr std::uint64_t chars_to_uint64 (char const c1, char const c2, char const c3,
                                                        char const c4, char const c5, char const c6,
                                                        char const c7, char const c8) noexcept {
            return static_cast<std::uint64_t> (c1) | static_cast<std::uint64_t> (c2) << 8U |
                   static_cast<std::uint64_t> (c3) << 16U | static_cast<std::uint64_t> (c4) << 24U |
                   static_cast<std::uint64_t> (c5) << 32U | static_cast<std::uint64_t> (c6) << 40U |
                   static_cast<std::uint64_t> (c7) << 48U | static_cast<std::uint64_t> (c8) << 56U;
        }
    };

    // Assert the size, offset, and alignment of the structure and its fields to ensure file format
    // compatibility across compilers and hosts.
    PSTORE_STATIC_ASSERT (offsetof (lock_block, vacuum_lock) == 0);
    PSTORE_STATIC_ASSERT (offsetof (lock_block, transaction_lock) == 8);
    PSTORE_STATIC_ASSERT (alignof (lock_block) == 8);
    PSTORE_STATIC_ASSERT (sizeof (lock_block) == 16);

    constexpr auto leader_size = sizeof (header) + sizeof (lock_block);

    class database;

    namespace index {

#define PSTORE_INDICES                                                                             \
    X (compilation)                                                                                \
    X (debug_line_header)                                                                          \
    X (fragment)                                                                                   \
    X (name)                                                                                       \
    X (path)                                                                                       \
    X (write)

        struct header_block;
    } // namespace index

    /// \brief The transaction footer structure.
    /// An copy of this structure is written to the data store at the end of each transaction block.
    /// pstore::header::footer_pos holds the address of the latest _complete_ instance and is
    /// updated once a transaction has been completely written to memory. Once written it is
    /// read-only.
    struct trailer {
        static std::array<std::uint8_t, 8> const default_signature1;
        static std::array<std::uint8_t, 8> const default_signature2;

        bool crc_is_valid () const noexcept;
        bool signature_is_valid () const noexcept;

        /// Checks that the address given by 'pos' appears to point to a valid transaction trailer.
        /// Raises exception::footer_corrupt if not.
        static bool validate (database const & db, typed_address<trailer> pos);

        /// Computes the trailer's CRC value.
        std::uint32_t get_crc () const noexcept;


#define X(a) a,
        // Note that the first enum member must have the value 0 or flush_indices() will need to
        // change.
        enum class indices : unsigned { PSTORE_INDICES last };
#undef X
        using index_records_array =
            std::array<typed_address<index::header_block>,
                       static_cast<std::underlying_type<indices>::type> (indices::last)>;

        /// Represents the portion of the trailer structure which is covered by the computed CRC
        /// value.
        struct body {
            std::array<std::uint8_t, 8> signature1 = default_signature1;
            std::atomic<std::uint32_t> generation{0};
            std::uint32_t unused1{0};

            /// The number of bytes contained by this transaction. The value does not include the
            /// size of the footer record.
            std::atomic<std::uint64_t> size{0};

            /// The time at which the transaction was committed, in milliseconds since the epoch.
            std::atomic<std::uint64_t> time{0};

            /// A pointer to the previous generation. This field forms a reverse linked list which
            /// allows a consumer to enumerate the generations contained within the store and to
            /// "sync" to a specific number.
            typed_address<trailer> prev_generation = typed_address<trailer>::null ();

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

    // Assert the size, offset, and alignment of the structure and its fields to ensure file format
    // compatibility across compilers and hosts.
    PSTORE_STATIC_ASSERT (offsetof (trailer::body, signature1) == 0);
    PSTORE_STATIC_ASSERT (offsetof (trailer::body, generation) == 8);
    PSTORE_STATIC_ASSERT (offsetof (trailer::body, unused1) == 12);
    PSTORE_STATIC_ASSERT (offsetof (trailer::body, size) == 16);
    PSTORE_STATIC_ASSERT (offsetof (trailer::body, time) == 24);
    PSTORE_STATIC_ASSERT (offsetof (trailer::body, prev_generation) == 32);
    PSTORE_STATIC_ASSERT (offsetof (trailer::body, index_records) == 40);
    PSTORE_STATIC_ASSERT (offsetof (trailer::body, unused2) == 88);
    PSTORE_STATIC_ASSERT (offsetof (trailer::body, unused3) == 92);
    PSTORE_STATIC_ASSERT (alignof (trailer::body) == 8);
    PSTORE_STATIC_ASSERT (sizeof (trailer::body) == 96);

    PSTORE_STATIC_ASSERT (offsetof (trailer, a) == 0);
    PSTORE_STATIC_ASSERT (offsetof (trailer, crc) == 96);
    PSTORE_STATIC_ASSERT (offsetof (trailer, signature2) == 104);
    PSTORE_STATIC_ASSERT (alignof (trailer) == 8);
    PSTORE_STATIC_ASSERT (sizeof (trailer) == 112);

} // namespace pstore
#endif // PSTORE_CORE_FILE_HEADER_HPP
