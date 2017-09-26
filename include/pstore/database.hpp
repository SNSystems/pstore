//*      _       _        _                     *
//*   __| | __ _| |_ __ _| |__   __ _ ___  ___  *
//*  / _` |/ _` | __/ _` | '_ \ / _` / __|/ _ \ *
//* | (_| | (_| | || (_| | |_) | (_| \__ \  __/ *
//*  \__,_|\__,_|\__\__,_|_.__/ \__,_|___/\___| *
//*                                             *
//===- include/pstore/database.hpp ----------------------------------------===//
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
/// \file database.hpp

#ifndef PSTORE_DATABASE_HPP
#define PSTORE_DATABASE_HPP (1)

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <vector>

#include "pstore_support/error.hpp"
#include "pstore_support/file.hpp" // for file_handle

#include "pstore/file_header.hpp" // for address
#include "pstore/fnv.hpp"
#include "pstore/hamt_map_fwd.hpp"
#include "pstore/head_revision.hpp"
#include "pstore/make_unique.hpp"
#include "pstore/memory_mapper.hpp" // for memory_mapper
#include "pstore/region.hpp"
#include "pstore/shared_memory.hpp"
#include "pstore/storage.hpp"
#include "pstore/uint128.hpp"
#include "pstore/vacuum_intf.hpp"

#include "serialize/archive.hpp"
#include "serialize/standard_types.hpp"
#include "serialize/types.hpp"

namespace pstore {
    /// Calculate the value that must be added to 'v' in order that it has the alignment
    /// given by 'align'.
    ///
    /// \param v      The value to be aligned.
    /// \param align  The alignment required for 'v'.
    /// \returns  The value that must be added to 'v' in order that it has the alignment given by
    /// 'align'.
    template <typename Ty>
    inline Ty calc_alignment (Ty v, std::size_t align) {
        return (align == 0U) ? 0U : ((v + align - 1U) & ~(align - 1U)) - v;
    }

    /// Calculate the value that must be added to 'v' in order for it to have the alignment required
    /// by type Ty.
    ///
    /// \param v  The value to be aligned.
    /// \returns  The value that must be added to 'v' in order that it has the alignment required by type Ty.
    template <typename Ty>
    inline Ty calc_alignment (Ty v) {
        return calc_alignment (v, alignof (Ty));
    }


    template <typename LockGuard>
    class transaction;

    namespace index {
        using write_index = hamt_map<std::string, record>;

        using uint128 = ::pstore::uint128;
        using digest = uint128;
        struct u128_hash {
            std::uint64_t operator() (digest const & v) const {
                return v.high ();
            }
        };
        using digest_index = hamt_map<digest, record, u128_hash>;

        // Note: Since uuid byte 6 represents version and byte 8 represents variant, we don't want
        // to use these two bytes. Therefore, we use the array[0-3,12-15] to construct the uint64_t.
        struct uuid_hash {
            std::uint64_t operator() (pstore::uuid const & v) const {
                auto & uuid_bytes = v.array ();
                return static_cast<std::uint64_t> (uuid_bytes[0]) << (8 * 7) |
                       static_cast<std::uint64_t> (uuid_bytes[1]) << (8 * 6) |
                       static_cast<std::uint64_t> (uuid_bytes[2]) << (8 * 5) |
                       static_cast<std::uint64_t> (uuid_bytes[3]) << (8 * 4) |
                       static_cast<std::uint64_t> (uuid_bytes[12]) << (8 * 3) |
                       static_cast<std::uint64_t> (uuid_bytes[13]) << (8 * 2) |
                       static_cast<std::uint64_t> (uuid_bytes[14]) << (8 * 1) |
                       static_cast<std::uint64_t> (uuid_bytes[15]);
            }
        };
        using ticket_index = hamt_map<pstore::uuid, record, uuid_hash>;

        using name_index = hamt_set<std::string, fnv_64a_hash>;
    } // namespace index


    namespace serialize {
        /// \brief A serializer for uint128
        template <>
        struct serializer<uint128> {

            /// \brief Writes an individual uint128 instance to an archive.
            ///
            /// \param archive  The archive to which the span will be written.
            /// \param v        The object value which is to be written.
            template <typename Archive>
            static auto write (Archive & archive, uint128 const & v) ->
                typename Archive::result_type {
                return archive.put (v);
            }

            /// \brief Writes a span of uint128 instances to an archive.
            ///
            /// \param archive  The archive to which the span will be written.
            /// \param span     The span which is to be written.
            template <typename Archive, typename SpanType>
            static auto writen (Archive & archive, SpanType span) -> typename Archive::result_type {
                static_assert (std::is_same<typename SpanType::element_type, uint128>::value,
                               "span type does not match the serializer type");
                return archive.putn (span);
            }

            /// \brief Reads a uint128 value from an archive.
            ///
            /// \param archive  The archive from which the value will be read.
            /// \param out      A reference to uninitialized memory into which a uint128 will be
            /// read.
            template <typename Archive>
            static void read (Archive & archive, uint128 & out) {
                assert (reinterpret_cast<std::uintptr_t> (&out) % alignof (uint128) == 0);
                archive.get (out);
            }

            /// \brief Reads a span of uint128 values from an archive.
            ///
            /// \param archive  The archive from which the value will be read.
            /// \param span     A span pointing to uninitialized memory
            template <typename Archive, typename Span>
            static void readn (Archive & archive, Span span) {
                static_assert (std::is_same<typename Span::element_type, uint128>::value,
                               "span type does not match the serializer type");
                details::getn_helper::getn (archive, span);
            }
        };

        /// \brief A serializer for uuid
        template <>
        struct serializer<pstore::uuid> {

            /// \brief Writes an individual uuid instance to an archive.
            ///
            /// \param archive  The archive to which the span will be written.
            /// \param v        The object value which is to be written.
            template <typename Archive>
            static auto write (Archive & archive, pstore::uuid const & v) ->
                typename Archive::result_type {
                return archive.put (v);
            }

            /// \brief Reads a uuid value from an archive.
            ///
            /// \param archive  The archive from which the value will be read.
            /// \param out      A reference to uninitialized memory into which a uuid will be
            /// read.
            template <typename Archive>
            static void read (Archive & archive, pstore::uuid & out) {
                assert (reinterpret_cast<std::uintptr_t> (&out) % alignof (pstore::uuid) == 0);
                archive.get (out);
            }
        };

    } // namespace serialize
} // namespace pstore


//*       _       _        _                      *
//*    __| | __ _| |_ __ _| |__   __ _ ___  ___   *
//*   / _` |/ _` | __/ _` | '_ \ / _` / __|/ _ \  *
//*  | (_| | (_| | || (_| | |_) | (_| \__ \  __/  *
//*   \__,_|\__,_|\__\__,_|_.__/ \__,_|___/\___|  *
//*                                               *
namespace pstore {

    class heartbeat;

    class database {
    public:
        explicit database (std::string const & path, bool writable,
                           bool access_tick_enabled = true);

        /// Create a database from a pre-opened file. This interface is intended to enable
        /// the database class to be unit tested.
        template <typename File>
        explicit database (std::shared_ptr<File> file,
                           std::unique_ptr<system_page_size_interface> && ps,
                           std::unique_ptr<region::factory> && region_factory,
                           bool access_tick_enabled = true);

        template <typename File>
        explicit database (std::shared_ptr<File> file, bool access_tick_enabled = true)
                : database (file, std::make_unique<system_page_size> (),
                            region::get_factory (file, storage::full_region_size,
                                                 storage::min_region_size),
                            access_tick_enabled) {}

        virtual ~database () noexcept;

        // Standard move behaviour.
        database (database &&) = default;
        database & operator= (database &&) = default;

        // No assignment or copying.
        database (database const &) = delete;
        database & operator= (database const &) = delete;


        /// \brief Returns the logical size of the data store.
        /// The local size of the data store is the number of bytes used, including both the data
        /// and meta-data. This may be less than the size of the physical disk file.
        std::uint64_t size () const {
            return size_.logical_size ();
        }

        /// \brief Returns the path of the file in which this database is contained.
        std::string path () const {
            return storage_.file ()->path ();
        }

        ///@{
        /// \brief Returns the file in which this database is contained.
        file::file_base const * file () const {
            return storage_.file ();
        }
        file::file_base * file () {
            return storage_.file ();
        }
        ///@}

        /// \brief Constructs the basic data store structures in an empty file.
        /// On return, the file will contain the correct header and a single, empty, transaction.
        static void build_new_store (file::file_base & file);

        /// \brief Update to a specified revision of the data.
        void sync (unsigned revision = head_revision);
        bool is_synced_to_head () const;

        static bool small_files_enabled () {
            return region::small_files_enabled ();
        }

        std::unique_lock<file::range_lock> * upgrade_to_write_lock ();
        std::time_t latest_time () const {
            auto lt = this->file ()->latest_time ();
#ifdef _WIN32
            lt = std::max (lt, this->get_shared ()->time.load ());
#endif
            return lt;
        }

        ///@{
        std::shared_ptr<void const> getro (record const & r) const;
        std::shared_ptr<void const> getro (address addr, std::size_t size) const;
        template <typename Ty>
        std::shared_ptr<Ty const> getro (address addr) const;
        template <typename Ty>
        std::shared_ptr<Ty const> getro (address addr, std::size_t size) const;
        ///@}

        ///@{
        std::shared_ptr<void> getrw (record const & r);
        std::shared_ptr<void> getrw (address addr, std::size_t size);
        template <typename T>
        auto getrw (address sop) -> std::shared_ptr<T>;
        template <typename T>
        auto getrw (address sop, std::size_t elements) -> std::shared_ptr<T>;
        ///@}

        // (Virtual for mocking.)
        virtual auto get (address addr, std::size_t size, bool initialized, bool writable) const
            -> std::shared_ptr<void const>;

        ///@{
        enum class vacuum_mode {
            disabled,
            immediate,
            background,
        };
        void set_vacuum_mode (vacuum_mode mode) {
            vacuum_mode_ = mode;
        }
        vacuum_mode get_vacuum_mode () const {
            return vacuum_mode_;
        }
        ///@}


        /// For unit testing
        class storage const & storage () const {
            return storage_;
        }

        void close ();

        address footer_pos () const {
            return size_.footer_pos ();
        }


        /// \brief Returns the name of the store's synchronisation object.
        ///
        /// This is set of 20 letters (`sync_name_length`) from a 32 character alphabet whose value
        /// is derived from the store's UUID. Assuming a truly uniform distribution, we have a
        /// collision probability of 1/32^20 which should be more than small enough for our
        /// purposes.
        std::string get_sync_name () const {
            assert (sync_name_.length () > 0);
            return sync_name_;
        }

        std::string shared_memory_name () const {
            return this->get_sync_name () + ".pst";
        }

        /// Returns a pointer to the write index, loading it from the store on first access. If
        /// 'create' is false and the index does not already exist then nullptr is returned.
        pstore::index::write_index * get_write_index (bool create = true);

        /// Returns a pointer to the digest index, loading it from the store on first access. If
        /// 'create' is false and the index does not already exist then nullptr is returned.
        pstore::index::digest_index * get_digest_index (bool create = true);

        /// Returns a pointer to the ticket index, loading it from the store on first access. If
        /// 'create' is false and the index does not already exist then nullptr is returned.
        pstore::index::ticket_index * get_ticket_index (bool create = true);

        /// Returns a pointer to the name index, loading it from the store on first access. If
        /// 'create' is false and the index does not already exist then nullptr is returned.
        pstore::index::name_index * get_name_index (bool create = true);



        /// Updates logical_size_ and potentially, the regions_ and sat_ arrays.
        ///
        /// \param lock   A reference to a write lock held on this database. This parameter is here
        ///               simply to ensure that we really do have a write lock when messing with
        ///               the database file.
        /// \param bytes  The number of bytes to be allocated.
        /// \param align  The alignment of the allocated storage. Must be a power of 2.
        template <typename LockGuard>
        auto allocate (LockGuard & lock, std::uint64_t bytes, unsigned align) -> address;

        /// Call as part of completing a transaction. We update the database records to that
        /// the new footer is recorded.
        void set_new_footer (header * const head, address new_footer_pos);

        void protect (address first, address last) {
            storage_.protect (first, last);
        }


        /// \brief Returns true if CRC checks are enabled.
        ///
        /// The library uses simple CRC checks to ensure the validity of its internal
        /// data structures. When fuzz testing, these can be disabled to increase the
        /// probability of the fuzzer uncovering a real bug. Always enabled otherwise.
        static bool crc_checks_enabled ();


        shared const * get_shared () const;
        shared * get_shared ();

    protected:
        virtual address allocate (std::uint64_t bytes, unsigned align);

    private:
        class storage storage_;
        file::range_lock range_lock_;
        std::unique_lock<file::range_lock> lock_;

        vacuum_mode vacuum_mode_ = vacuum_mode::disabled;
        bool modified_ = false;
        bool closed_ = false;

        /// The current logical end-of-file, which may be less than the physical end-of-file due to
        /// the memory manager on Windows requiring that the file backing a memory mapped region be
        /// at least as large as that region.
        ///
        /// After opening the file or performing a sync() operation, this will point just beyond
        /// the transaction's file footer. If a write transaction is active, then this becomes the
        /// point at which new data is written; when the transaction is complete, a new footer will
        /// be written at this location.

        // TODO: should this be a member of 'transaction'?
        class sizes {
        public:
            sizes () noexcept
                    : footer_pos_{address::null ()}
                    , logical_{0} {}
            explicit sizes (address footer_pos) noexcept
                    : footer_pos_{footer_pos}
                    , logical_{footer_pos_.absolute () + sizeof (trailer)} {}

            address footer_pos () const {
                return footer_pos_;
            }
            std::uint64_t logical_size () const {
                return logical_;
            }

            void update_footer_pos (address new_footer_pos) {
                assert (new_footer_pos.absolute () >= sizeof (header));
                footer_pos_ = new_footer_pos;
                logical_ = std::max (logical_, footer_pos_.absolute () + sizeof (trailer));
            }

            void update_logical_size (std::uint64_t new_logical_size) {
                assert (new_logical_size >= footer_pos_.absolute () + sizeof (trailer));
                logical_ = std::max (logical_, new_logical_size);
            }

        private:
            address footer_pos_;

            /// This value tracks space as it's appended to the file.
            std::uint64_t logical_;
        };
        sizes size_;

        std::array<std::unique_ptr<index::index_base>,
                   static_cast<unsigned> (trailer::indices::last)>
            indices_;
        std::string sync_name_;
        static constexpr auto const sync_name_length = std::size_t{20};


        shared_memory<shared> shared_;
        std::shared_ptr<heartbeat> heartbeat_;

        /// Clears the index cache: the next time that an index is requested it will be read from
        /// the disk. Used after a sync() operation has changed the current database view.
        void clear_index_cache ();

        template <typename IndexType>
        auto get_index (enum trailer::indices which, bool create) -> IndexType *;

        /// Returns a block of data from the store which spans more than one region. A fresh block
        /// of memory is allocated to which blocks of data from the store are copied. If a writable
        /// pointer is requested, the data will be copied back to the store when the pointer is
        /// released.
        auto get_spanning (address addr, std::size_t size, bool initialized, bool writable) const
            -> std::shared_ptr<void const>;



        template <typename File>
        static address get_footer_pos (File & file);

        std::shared_ptr<trailer const> get_footer () const {
            return this->getro<trailer> (this->footer_pos ());
        }

        static std::string build_sync_name (header const & header);


        /// \param new_size  The amount of memory-mapped that is required. If necessary, additional
        ///                  space will be mapped and the underlying file size increased.
        void map_bytes (std::uint64_t new_size);

        /// Opens a database file, creating it if it does not exist. On return the global mutex
        /// is held on the file.
        ///
        /// \param path  The path to the file to be opened.
        /// \param writable  True if the resulting database should be writable: the file will
        ///                  be created if it does not already exist. If false and the file does
        ///                  not exist, the returned file handle will be a nullptr.
        /// \returns A file handle to the opened database file which may be null if the file could
        /// not be opened.
        static auto open (std::string const & path, bool writable)
            -> std::shared_ptr<file::file_handle>;

        /// Completes the initialization of a database instance. This function should be called by
        /// all of the class constructors.
        void finish_init (bool access_tick_enabled);
    };


    // (ctor)
    // ~~~~~~
    template <typename File>
    database::database (std::shared_ptr<File> file,
                        std::unique_ptr<system_page_size_interface> && page_size,
                        std::unique_ptr<region::factory> && region_factory,
                        bool access_tick_enabled)
            : storage_{std::move (file), std::move (page_size), std::move (region_factory)}
            , size_{database::get_footer_pos (*file)} {

        this->finish_init (access_tick_enabled);
    }


    // get_footer_pos [static]
    // ~~~~~~~~~~~~~~
    template <typename File>
    auto database::get_footer_pos (File & file) -> address {
        static_assert (std::is_base_of<file::file_base, File>::value,
                       "File type must be derived from file::file_base");
        assert (file.is_open ());

        header h;
        file.seek (0);
        file.read (&h);

        if (h.a.signature1 != header::file_signature1 ||
            h.a.signature2 != header::file_signature2) {

            raise (error_code::header_corrupt, file.path ());
        }

        if (h.a.header_size != sizeof (class header) || h.a.version[0] != header::major_version ||
            h.a.version[1] != header::minor_version) {

            raise (error_code::header_version_mismatch, file.path ());
        }

        if (!h.is_valid ()) {
            raise (pstore::error_code::header_corrupt, file.path ());
        }

        address const footer_pos = h.footer_pos.load ();
        std::uint64_t const footer_offset = footer_pos.absolute ();
        std::uint64_t const file_size = file.size ();
        if (footer_offset < sizeof (header) || file_size < sizeof (header) + sizeof (trailer) ||
            footer_offset > file_size - sizeof (trailer)) {

            raise (error_code::header_corrupt, file.path ());
        }

        return footer_pos;
    }




    // allocate
    // ~~~~~~~~
    template <typename LockGuard>
    auto database::allocate (LockGuard &, std::uint64_t bytes, unsigned align) -> address {
        return this->allocate (bytes, align);
    }


    // getrw
    // ~~~~~
    template <typename Ty>
    auto database::getrw (address addr) -> std::shared_ptr<Ty> {
        assert (addr.absolute () % alignof (Ty) == 0);
        auto ptr = this->getrw (addr, sizeof (Ty));
        return std::static_pointer_cast<Ty> (ptr);
    }
    template <typename Ty>
    auto database::getrw (address addr, std::size_t elements) -> std::shared_ptr<Ty> {
        assert (addr.absolute () % alignof (Ty) == 0);
        STATIC_ASSERT (std::is_standard_layout<Ty>::value);
        auto const size = sizeof (Ty) * elements;
        std::shared_ptr<void> result = this->getrw (addr, size);
        return std::static_pointer_cast<Ty> (result);
    }
    inline auto database::getrw (record const & r) -> std::shared_ptr<void> {
        return this->getrw (r.addr, r.size);
    }
    inline auto database::getrw (address addr, std::size_t size) -> std::shared_ptr<void> {
        return std::const_pointer_cast<void> (
            this->get (addr, size, true /*initialized*/, true /*writable*/));
    }


    // getro
    // ~~~~~
    inline auto database::getro (record const & r) const -> std::shared_ptr<void const> {
        return this->getro (r.addr, r.size);
    }
    template <typename Ty>
    inline auto database::getro (address addr) const -> std::shared_ptr<Ty const> {
        assert (addr.absolute () % alignof (Ty) == 0);
        return std::static_pointer_cast<Ty const> (this->getro (addr, sizeof (Ty)));
    }
    template <typename Ty>
    inline auto database::getro (address addr, std::size_t elements) const
        -> std::shared_ptr<Ty const> {
        assert (addr.absolute () % alignof (Ty) == 0);
        auto const size = sizeof (Ty) * elements;
        return std::static_pointer_cast<Ty const> (this->getro (addr, size));
    }
    inline auto database::getro (address addr, std::size_t size) const
        -> std::shared_ptr<void const> {
        return this->get (addr, size, true /*initialized*/, false /*writable*/);
    }

} // namespace pstore

#endif // PSTORE_DATABASE_HPP
// eof: include/pstore/database.hpp
