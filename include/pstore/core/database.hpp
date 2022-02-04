//===- include/pstore/core/database.hpp -------------------*- mode: C++ -*-===//
//*      _       _        _                     *
//*   __| | __ _| |_ __ _| |__   __ _ ___  ___  *
//*  / _` |/ _` | __/ _` | '_ \ / _` / __|/ _ \ *
//* | (_| | (_| | || (_| | |_) | (_| \__ \  __/ *
//*  \__,_|\__,_|\__\__,_|_.__/ \__,_|___/\___| *
//*                                             *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file database.hpp

#ifndef PSTORE_CORE_DATABASE_HPP
#define PSTORE_CORE_DATABASE_HPP

#include "pstore/adt/sstring_view.hpp"
#include "pstore/core/file_header.hpp"
#include "pstore/core/hamt_map_fwd.hpp"
#include "pstore/core/storage.hpp"
#include "pstore/core/vacuum_intf.hpp"
#include "pstore/os/shared_memory.hpp"
#include "pstore/support/head_revision.hpp"

namespace pstore {

    template <typename T>
    using unique_deleter = void (*) (T * p);
    template <typename T>
    using unique_pointer = std::unique_ptr<T, unique_deleter<T>>;

    /// A deleter function for use with unique_pointer<>. This version of the function is used
    /// when the requested storage lies entirely within a single allocated region.
    template <typename T>
    inline void deleter_nop (T *) noexcept {}
    /// A deleter function for use with unique_pointer<>. This version of the function is used
    /// to recover memory for spanning pointers.
    ///
    /// \param p  Points to the memory to be freed.
    template <typename T>
    void deleter (T * const p) noexcept {
        delete[] p;
    }

    template <>
    inline void deleter<void> (void * const p) noexcept {
        deleter (reinterpret_cast<std::uint8_t *> (p));
    }
    template <>
    inline void deleter<void const> (void const * const p) noexcept {
        deleter (reinterpret_cast<std::uint8_t const *> (p));
    }

    template <typename To, typename From>
    unique_pointer<To> unique_pointer_cast (unique_pointer<From> && p) {
        // Note that we know that the cast of the deleter function is safe. This function will
        // either be deleter_nop (in the vast majority of instances) or deleter<> whose
        // underlying memory is always allocated as uint8_t[] by get_spanningu().
        return unique_pointer<To const> (
            reinterpret_cast<To const *> (p.release ()),
            reinterpret_cast<unique_deleter<To const>> (p.get_deleter ()));
    }



    //*       _       _        _                      *
    //*    __| | __ _| |_ __ _| |__   __ _ ___  ___   *
    //*   / _` |/ _` | __/ _` | '_ \ / _` / __|/ _ \  *
    //*  | (_| | (_| | || (_| | |_) | (_| \__ \  __/  *
    //*   \__,_|\__,_|\__\__,_|_.__/ \__,_|___/\___|  *
    //*                                               *

    class heartbeat;

    class database {
    public:
        enum class access_mode { read_only, writable, writeable_no_create };

        /// Creates a database instance give the path of the file to use.
        ///
        /// \param path  The path of the file containing the database.
        /// \param am  The requested access mode. If the file does not exist and writable access is
        /// requested, a new empty database is created. If read-only access is requested and the
        /// file does not exist, an error is raised.
        explicit database (std::string const & path, access_mode am,
                           bool access_tick_enabled = true);

        /// Create a database from a pre-opened file. This interface is intended to enable
        /// the database class to be unit tested.
        template <typename File>
        explicit database (std::shared_ptr<File> file,
                           std::unique_ptr<system_page_size_interface> && page_size,
                           std::unique_ptr<region::factory> && region_factory,
                           bool access_tick_enabled = true);

        template <typename File>
        explicit database (std::shared_ptr<File> file, bool access_tick_enabled = true)
                : database (file, std::make_unique<system_page_size> (),
                            region::get_factory (file, storage::full_region_size,
                                                 storage::min_region_size),
                            access_tick_enabled) {}

        database (database &&) = delete;
        database (database const &) = delete;

        virtual ~database () noexcept;

        database & operator= (database &&) = delete;
        database & operator= (database const &) = delete;


        /// \brief Returns the logical size of the data store.
        /// The local size of the data store is the number of bytes used, including both the data
        /// and meta-data. This may be less than the size of the physical disk file.
        std::uint64_t size () const noexcept { return size_.logical_size (); }

        /// \brief Returns the path of the file in which this database is contained.
        std::string path () const { return storage_.file ()->path (); }

        ///@{
        /// \brief Returns the file in which this database is contained.
        file::file_base const * file () const { return storage_.file (); }
        file::file_base * file () { return storage_.file (); }
        ///@}

        /// \brief Constructs the basic data store structures in an empty file.
        /// On return, the file will contain the correct header and a single, empty, transaction.
        static void build_new_store (file::file_base & file);

        /// \brief Update to a specified revision of the data.
        void sync (unsigned revision = head_revision);

        /// \brief Returns the address of the footer of a specified revision.
        ///
        /// \param revision  The revision number. Should not be pstore::head_revision and should be
        /// less than or equal to the current revision number (as returned by
        /// database::get_current_revision(). In this event, an unknown_revision error is raised.
        ///
        /// \note This is a const member function and therefore cannot "see" revisions later than
        /// the old currently synced because to do so may require additional space to be mapped.
        typed_address<trailer> older_revision_footer_pos (unsigned revision) const;

        static constexpr bool small_files_enabled () noexcept {
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
        bool is_writable () const noexcept { return storage_.file ()->is_writable (); }

        ///@{
        /// Load a block of data starting at address \p addr and of \p size bytes.
        ///
        /// \param addr The starting address of the data block to be loaded.
        /// \param size The size of the data block to be loaded.
        /// \return A read-only pointer to the loaded data.
        std::shared_ptr<void const> getro (address const addr, std::size_t const size) const {
            return this->get (addr, size, true /*initialized*/, false /*writable*/);
        }

        unique_pointer<void const> getrou (address const addr, std::size_t const size) const {
            return this->getu (addr, size, true /*initialized*/);
        }
        /// Load a block of data starting at the address and size specified by \p ex and return an
        /// immutable shared pointer.
        ///
        /// \tparam T  The data type to be loaded.
        /// \param ex The extent of of the data to be loaded.
        /// \return A read-only pointer to the loaded data.
        template <typename T,
                  typename = typename std::enable_if_t<std::is_standard_layout<T>::value>>
        std::shared_ptr<T const> getro (extent<T> const & ex) const;

        /// Load a block of data starting at the address and size specified by \p ex and return an
        /// immutable unique pointer.
        ///
        /// \tparam T  The data type to be loaded.
        /// \param ex The extent of of the data to be loaded.
        /// \return A read-only pointer to the loaded data.
        template <typename T,
                  typename = typename std::enable_if_t<std::is_standard_layout<T>::value>>
        unique_pointer<T const> getrou (extent<T> const & ex) const;

        /// Returns a shared-pointer to a immutable instance of type T.
        ///
        /// \tparam T  The type to be loaded. Must be standard-layout.
        /// \param addr The address at which the data begins.
        /// \return A read-only pointer to the loaded data.
        template <typename T,
                  typename = typename std::enable_if_t<std::is_standard_layout<T>::value>>
        std::shared_ptr<T const> getro (typed_address<T> const addr) const {
            return this->getro (addr, std::size_t{1});
        }

        /// Returns a unique-pointer to a immutable instance of type T.
        ///
        /// \tparam T  The type to be loaded. Must be standard-layout.
        /// \param addr The address at which the data begins.
        /// \return A read-only pointer to the loaded data.
        template <typename T,
                  typename = typename std::enable_if_t<std::is_standard_layout<T>::value>>
        unique_pointer<T const> getrou (typed_address<T> const addr) const {
            return this->getrou (addr, std::size_t{1});
        }

        /// Returns a shared-pointer to a read-only array of instances of type T.
        ///
        /// \tparam T  The type to be loaded. Must be standard-layout.
        /// \param addr The address at which the data begins.
        /// \param elements The number of elements in the T[] array.
        /// \return A read-only pointer to the loaded data.
        template <typename T,
                  typename = typename std::enable_if_t<std::is_standard_layout<T>::value>>
        std::shared_ptr<T const> getro (typed_address<T> const addr,
                                        std::size_t const elements) const {
            if (addr.to_address ().absolute () % alignof (T) != 0) {
                raise (error_code::bad_alignment);
            }
            return std::static_pointer_cast<T const> (
                this->getro (addr.to_address (), sizeof (T) * elements));
        }

        /// Returns a unique-pointer to a read-only array of instances of type T.
        ///
        /// \tparam T  The type to be loaded. Must be standard-layout.
        /// \param addr The address at which the data begins.
        /// \param elements The number of elements in the T[] array.
        /// \return A read-only pointer to the loaded data.
        template <typename T,
                  typename = typename std::enable_if_t<std::is_standard_layout<T>::value>>
        unique_pointer<T const> getrou (typed_address<T> const addr,
                                        std::size_t const elements) const;
        ///@}

        ///@{
        /// A collection of functions which obtain a non-const pointer to database storage.
        /// These functions should only be called by the transaction code. Data outside of
        /// an open transaction is always read-only and the underlying memory is marked
        /// read-only. Writing through the pointer returned by these functions may cause client code
        /// to crash if the address lies outside the range of the expected range.

        /// Load a block of data starting at address \p addr and of \p size bytes.
        ///
        /// \param addr The starting address of the data block to be loaded.
        /// \param size The size of the data block to be loaded.
        /// \return A mutable pointer to the loaded data.
        std::shared_ptr<void> getrw (address const addr, std::size_t const size) {
            return std::const_pointer_cast<void> (
                this->get (addr, size, true /*initialized*/, true /*writable*/));
        }

        /// Loads a block of storage at the address and size given by \p ex.
        ///
        /// \param ex The extent of the data.
        /// \return A mutable pointer to the loaded data.
        template <typename T,
                  typename = typename std::enable_if<std::is_standard_layout<T>::value>::type>
        std::shared_ptr<T> getrw (extent<T> const & ex) {
            if (ex.addr.to_address ().absolute () % alignof (T) != 0) {
                raise (error_code::bad_alignment);
            }
            // Note that ex.size specifies the size in bytes of the data to be loaded, not the
            // number of elements of type T. For this reason we call the plain address version of
            // getro().
            return std::static_pointer_cast<T> (this->getrw (ex.addr.to_address (), ex.size));
        }

        /// Returns a pointer to a mutable instance of type T.
        ///
        /// \param addr The address at which the data begins.
        /// \return A mutable pointer to the loaded data.
        template <typename T,
                  typename = typename std::enable_if<std::is_standard_layout<T>::value>::type>
        std::shared_ptr<T> getrw (typed_address<T> addr) {
            return this->getrw<T> (addr, std::size_t{1});
        }

        /// Returns a pointer to a mutable array of instances of type T.
        ///
        /// \param addr The address at which the data begins.
        /// \param elements The number of elements in the T[] array.
        /// \return A mutable pointer to the loaded data.
        template <typename T,
                  typename = typename std::enable_if<std::is_standard_layout<T>::value>::type>
        std::shared_ptr<T> getrw (typed_address<T> const addr, std::size_t const elements) {
            if (addr.to_address ().absolute () % alignof (T) != 0) {
                raise (error_code::bad_alignment);
            }
            return std::static_pointer_cast<T> (
                this->getrw (addr.to_address (), sizeof (T) * elements));
        }
        ///@}

        // (Virtual for mocking.)
        virtual auto get (address addr, std::size_t size, bool initialized, bool writable) const
            -> std::shared_ptr<void const>;
        unique_pointer<void const> getu (address addr, std::size_t size, bool initialized) const;

        ///@{
        enum class vacuum_mode {
            disabled,
            immediate,
            background,
        };
        void set_vacuum_mode (vacuum_mode const mode) noexcept { vacuum_mode_ = mode; }
        vacuum_mode get_vacuum_mode () const noexcept { return vacuum_mode_; }
        ///@}

        /// For unit testing
        class storage const & storage () const noexcept {
            return storage_;
        }

        void close ();

        header const & get_header () const noexcept { return *header_; }
        typed_address<trailer> footer_pos () const noexcept { return size_.footer_pos (); }

        /// Returns the generation number to which the database is synced.
        /// \note This generation number doesn't count an open transaction.
        unsigned get_current_revision () const { return get_footer ()->a.generation.load (); }

        /// \brief Returns the name of the store's synchronisation object.
        ///
        /// This is set of 20 letters (`sync_name_length`) from a 32 character alphabet whose value
        /// is derived from the store's ID. Assuming a truly uniform distribution, we have a
        /// collision probability of 1/32^20 which should be more than small enough for our
        /// purposes.
        std::string get_sync_name () const {
            PSTORE_ASSERT (sync_name_.length () > 0);
            return sync_name_;
        }

        std::string shared_memory_name () const { return this->get_sync_name () + ".pst"; }

        /// \brief Appends an amount of storage to the database.
        ///
        /// As an append-only system, this function provides the means by which data is recorded in
        /// the underlying storage; it is responsible for increasing the amount of available storage
        /// when necessary.
        ///
        /// \note Before calling this function it is important that the global write-lock is held
        /// (usually through use of transaction<>). Failure to do so will cause race conditions
        /// between processes accessing the store.
        ///
        /// \param bytes  The number of bytes to be allocated.
        /// \param align  The alignment of the allocated storage. Must be a power of 2.
        /// \returns The address of the newly allocated storage.
        virtual address allocate (std::uint64_t bytes, unsigned align);

        virtual void truncate (std::uint64_t size);

        /// Call as part of completing a transaction. We update the database records to that
        /// the new footer is recorded.
        void set_new_footer (typed_address<trailer> new_footer_pos);

        void protect (address const first, address const last) { storage_.protect (first, last); }

        /// \brief Returns true if CRC checks are enabled.
        ///
        /// The library uses simple CRC checks to ensure the validity of its internal
        /// data structures. When fuzz testing, these can be disabled to increase the
        /// probability of the fuzzer uncovering a real bug. Always enabled otherwise.
        static bool crc_checks_enabled ();

        void set_id (uuid const & id) noexcept { header_->set_id (id); }

        shared const * get_shared () const;
        shared * get_shared ();

        /// \brief Returns a pointer to an index base.
        ///
        /// \param which  An index type from which the index will be read.
        //  \warning This function is dangerous. It returns a non-const index from a const
        //  database.
        std::shared_ptr<index::index_base> &
        get_mutable_index (enum pstore::trailer::indices const which) const {
            return indices_[static_cast<std::underlying_type<decltype (which)>::type> (which)];
        }
        std::shared_ptr<trailer const> get_footer () const {
            return this->getro (this->footer_pos ());
        }

    private:
        class storage storage_;
        std::shared_ptr<header> header_;
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
        class sizes {
        public:
            sizes () noexcept = default;
            constexpr explicit sizes (typed_address<trailer> const footer_pos) noexcept
                    : footer_pos_{footer_pos}
                    , logical_{footer_pos_.absolute () + sizeof (trailer)} {}

            typed_address<trailer> footer_pos () const noexcept { return footer_pos_; }
            std::uint64_t logical_size () const noexcept { return logical_; }

            void update_footer_pos (typed_address<trailer> const new_footer_pos) noexcept {
                PSTORE_ASSERT (new_footer_pos.absolute () >= leader_size);
                footer_pos_ = new_footer_pos;
                logical_ = std::max (logical_, footer_pos_.absolute () + sizeof (trailer));
            }

            void update_logical_size (std::uint64_t const new_logical_size) noexcept {
                PSTORE_ASSERT (new_logical_size >= footer_pos_.absolute () + sizeof (trailer));
                logical_ = std::max (logical_, new_logical_size);
            }

            void truncate_logical_size (std::uint64_t const new_logical_size) noexcept {
                PSTORE_ASSERT (new_logical_size >= footer_pos_.absolute () + sizeof (trailer));
                logical_ = new_logical_size;
            }

        private:
            typed_address<trailer> footer_pos_ = typed_address<trailer>::null ();

            /// This value tracks space as it's appended to the file.
            std::uint64_t logical_ = 0;
        };
        sizes size_;

        mutable std::array<std::shared_ptr<index::index_base>,
                           static_cast<unsigned> (trailer::indices::last)>
            indices_;
        std::string sync_name_;
        static constexpr auto const sync_name_length = std::size_t{20};


        shared_memory<shared> shared_;
        std::shared_ptr<heartbeat> heartbeat_;

        /// Clears the index cache: the next time that an index is requested it will be read from
        /// the disk. Used after a sync() operation has changed the current database view.
        void clear_index_cache ();

        /// Returns the lowest address from which a writable pointer can be obtained.
        address first_writable_address () const;

        /// Validates the arguments passed to one of the get()/getu() functions.
        ///
        /// \param addr  The start address of the data to be copied.
        /// \param size  The number of bytes of data to be copied.
        /// \param writable  True if the memory is to be writable.
        void check_get_params (address const addr, std::size_t const size,
                               bool const writable) const;

        /// Returns a block of data from the store which spans more than one region. A fresh block
        /// of memory is allocated to which blocks of data from the store are copied. If a writable
        /// pointer is requested, the data will be copied back to the store when the pointer is
        /// released.
        ///
        /// \param addr  The start address of the data to be copied.
        /// \param size  The number of bytes of data to be copied.
        /// \param initialized  True if the data must be populated from the store before being
        ///   returned.
        /// \param writable  True if the memory is to be writable.
        //  \returns  A copy of the data which will be flushed as necessary on release.
        std::shared_ptr<void const> get_spanning (address addr, std::size_t size, bool initialized,
                                                  bool writable) const;

        /// Returns a block of data from the store which spans more than one region. A fresh block
        /// of memory is allocated to which blocks of data from the store are copied.
        ///
        /// \param addr  The start address of the data to be copied.
        /// \param size  The number of bytes of data to be copied.
        /// \param initialized  True if the data must be populated from the store before being
        ///   returned.
        /// \returns  A copy of the data which will be freed on release.
        unique_pointer<void const> get_spanningu (address addr, std::size_t size,
                                                  bool initialized) const;

        template <typename File>
        static typed_address<trailer> get_footer_pos (File & file);

        static std::string build_sync_name (header const & header);


        /// \param new_size  The amount of memory-mapped that is required. If necessary, additional
        ///                  space will be mapped and the underlying file size increased.
        void map_bytes (std::uint64_t new_size);

        /// Opens a database file, creating it if it does not exist. On return the global mutex
        /// is held on the file.
        ///
        /// \param path  The path to the file to be opened.
        /// \param am  The database access mode. If writable, the file will be created if it does
        ///   not already exist. If read-only and the file does not exist, the returned file handle
        ///   will be a nullptr.
        /// \returns A file handle to the opened database file which may be null if the file could
        /// not be opened.
        static auto open (std::string const & path, access_mode am)
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
                        bool const access_tick_enabled)
            : storage_{std::move (file), std::move (page_size), std::move (region_factory)}
            , size_{database::get_footer_pos (*file)} {

        this->finish_init (access_tick_enabled);
    }

    // getro
    // ~~~~~
    template <typename T, typename>
    std::shared_ptr<T const> database::getro (extent<T> const & ex) const {
        if (ex.addr.to_address ().absolute () % alignof (T) != 0) {
            raise (error_code::bad_alignment);
        }
        // Note that ex.size specifies the size in bytes of the data to be loaded, not the
        // number of elements of type T. For this reason we call the plain address version of
        // getro().
        return std::static_pointer_cast<T const> (this->getro (ex.addr.to_address (), ex.size));
    }

    // getrou
    // ~~~~~~
    template <typename T, typename>
    auto database::getrou (extent<T> const & ex) const -> unique_pointer<T const> {
        if (ex.addr.to_address ().absolute () % alignof (T) != 0) {
            raise (error_code::bad_alignment);
        }
        // ex.size specifies the size in bytes of the data to be loaded, not the number of elements
        // of type T. For this reason we call the plain address version of getro().
        return unique_pointer_cast<T const> (this->getrou (ex.addr.to_address (), ex.size));
    }

    template <typename T, typename>
    auto database::getrou (typed_address<T> const addr, std::size_t const elements) const
        -> unique_pointer<T const> {
        if (addr.to_address ().absolute () % alignof (T) != 0) {
            raise (error_code::bad_alignment);
        }
        return unique_pointer_cast<T const> (
            this->getrou (addr.to_address (), sizeof (T) * elements));
    }

    // get footer pos [static]
    // ~~~~~~~~~~~~~~
    template <typename File>
    auto database::get_footer_pos (File & file) -> typed_address<trailer> {
        static_assert (std::is_base_of<file::file_base, File>::value,
                       "File type must be derived from file::file_base");
        PSTORE_ASSERT (file.is_open ());

        std::aligned_storage<sizeof (header), alignof (header)>::type header_storage{};
        auto h = reinterpret_cast<header *> (&header_storage);
        file.seek (0);
        file.read (h);

        auto const dtor = [] (header * const p) { p->~header (); };
        std::unique_ptr<header, decltype (dtor)> deleter (h, dtor);

#if PSTORE_SIGNATURE_CHECKS_ENABLED
        if (h->a.signature1 != header::file_signature1 ||
            h->a.signature2 != header::file_signature2) {
            raise (error_code::header_corrupt, file.path ());
        }
#endif
        if (h->a.header_size != sizeof (class header) || h->a.version[0] != header::major_version ||
            h->a.version[1] != header::minor_version) {
            raise (error_code::header_version_mismatch, file.path ());
        }
        if (!h->is_valid ()) {
            raise (pstore::error_code::header_corrupt, file.path ());
        }

        typed_address<trailer> const result = h->footer_pos.load ();
        std::uint64_t const footer_offset = result.absolute ();
        std::uint64_t const file_size = file.size ();
        if (footer_offset < leader_size || file_size < leader_size + sizeof (trailer) ||
            footer_offset > file_size - sizeof (trailer)) {
            raise (error_code::header_corrupt, file.path ());
        }
        return result;
    }

} // namespace pstore

#endif // PSTORE_CORE_DATABASE_HPP
