//*   __ _ _       *
//*  / _(_) | ___  *
//* | |_| | |/ _ \ *
//* |  _| | |  __/ *
//* |_| |_|_|\___| *
//*                *
//===- include/pstore/os/file.hpp -----------------------------------------===//
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
/// \file file.hpp
/// \brief Cross platform file management functions and classes.

#ifndef PSTORE_OS_FILE_HPP
#define PSTORE_OS_FILE_HPP

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>

#ifdef _WIN32
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#endif

#include "pstore/config/config.hpp"
#include "pstore/support/array_elements.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {

    class memory_mapper;
    class in_memory_mapper;

    /// \brief The 'file' namespace contains all of the database file management functions
    /// and classes .
    namespace file {
        namespace details {

            // name_from_template
            // ~~~~~~~~~~~~~~~~~~
            /// \brief The name_from_template() function takes the given file name template and
            /// returns a string in which a portion of the template is overwritten to create a file
            /// name.
            ///
            /// The template may be any file name with some number of `Xs' appended to it, for
            /// example /tmp/temp.XXXXXX. The trailing `Xs' are replaced with a unique alphanumeric
            /// combination. The number of unique file names mktemp() can return depends on the
            /// number of `Xs' provided.
            ///
            /// \tparam RandomGenerator  A Callable object with the signature equivalent to
            /// std::function<unsigned(unsigned)>. The function's result must be in the range 0
            /// to the value given by the single parameter.
            ///
            /// \param tmpl  A template string which forms the basis for the result string. Any
            /// trailing X characters are replaced by characters derived from the result of the \p
            /// rng function.
            /// \param rng  A random number generator function which should return a value in the
            /// range [0,max).
            /// \return A string derived from the \p tmpl argument but with trailing 'X's replaced
            /// by random characters from an internal alphabet.
            ///
            /// \note This function is used on platforms that don't have a native implementation
            /// of the mkstemp() function.
            template <typename RandomGenerator>
            std::string name_from_template (std::string const & tmpl, RandomGenerator rng) {
                // Walk backwards looking for the start of the sequence of 'X'
                // characters that form the end of the template string.
                auto const it = std::find_if (tmpl.rbegin (), tmpl.rend (),
                                              [] (char const c) { return c != 'X'; });
                // Build the result string starting from the non-template characters.
                std::string path;
                path.reserve (tmpl.length ());
                std::copy (tmpl.begin (), it.base (), std::back_inserter (path));

                // Replace the sequence of 'X's with random characters.
                auto num_x = std::distance (it.base (), tmpl.end ());
                while (num_x--) {
                    static char const alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789_";
                    static constexpr auto length =
                        static_cast<unsigned> (array_elements (alphabet)) - 1U;
                    auto const index = rng (length);
                    assert (index >= 0 && index < length);
                    path.push_back (alphabet[index]);
                }
                assert (path.length () == tmpl.length ());
                return path;
            }

            // split
            // ~~~~~
            /// Unfortunately, the Win32 ReadFile() and WriteFile() functions accept size parameters
            /// whose type is DWORD whereas our API uses std::size_t. std::size_t is obviously
            /// 64-bits on a 64-bit host, but DWORD is always 32-bit. This function splits up the
            /// request into chunks which are no larger than the WidthType max value.
            ///
            /// \tparam WidthType  The integer type whose largest value determines the largest
            /// 'chunk' into which the size parameter can be split.
            /// \tparam PointeeType  The type of the elements of the `buffer` array.
            /// \tparam Function  A callable whose signature whose be equivalent to
            /// std::function<std::size_t(PointeeType*,WidthType)>. It is called for each chunk into
            /// which \p size is divided. The return value should be the number of bytes processed;
            /// the first argument is the first value to process; the second element is the number
            /// of contiguous elements to be processed.
            ///
            /// \param buffer The base address of the buffer to be processed by the provided
            /// callback.
            /// \param size  The total number of bytes to be processed.
            /// \param function  This function is called repeatedly to operate on each portion of
            /// the with the buffer parameter.
            /// \return The sum of the values returned by \p function.
            template <typename WidthType, typename PointeeType, typename Function>
            std::size_t split (PointeeType * buffer, std::size_t size, Function const & function) {

                static_assert (sizeof (PointeeType) == sizeof (std::uint8_t),
                               "PointeeType must be a byte wide type");
                static_assert (sizeof (std::size_t) >= sizeof (WidthType),
                               "WidthType must not be wider than size_t");
                static std::size_t const max = std::numeric_limits<WidthType>::max ();
                std::size_t result = 0;
                while (size > 0) {
                    auto const chunk_size = static_cast<WidthType> (std::min (max, size));
                    result += function (buffer, chunk_size);
                    buffer += chunk_size;
                    size -= chunk_size;
                }
                return result;
            }

            template <typename WidthType, typename Function>
            std::size_t split (void * const buffer, std::size_t const size,
                               Function const & function) {
                return split<WidthType> (static_cast<std::uint8_t *> (buffer), size, function);
            }
            template <typename WidthType, typename Function>
            std::size_t split (void const * const buffer, std::size_t const size,
                               Function const & function) {
                return split<WidthType> (static_cast<std::uint8_t const *> (buffer), size,
                                         function);
            }

        } // end namespace details


        class system_error : public std::system_error {
        public:
            system_error (std::error_code code, std::string const & user_message, std::string path);
            system_error (std::error_code code, gsl::czstring user_message, std::string path);

            system_error (system_error const &) = default;
            system_error (system_error &&) = default;

            ~system_error () noexcept override;

            system_error & operator= (system_error const &) = default;
            system_error & operator= (system_error &&) = default;

            std::string const & path () const noexcept { return path_; }

        private:
            std::string path_;

            template <typename MessageStringType>
            std::string message (MessageStringType const & user_message, std::string const & path);
        };


        /// \brief An abstract file class. Provides the interface for file access.
        class file_base {
        public:
            file_base (file_base &&) noexcept = default;
            file_base (file_base const &) = default;
            virtual ~file_base () noexcept;

            file_base & operator= (file_base &&) noexcept = default;
            file_base & operator= (file_base const &) = default;

            virtual bool is_open () const noexcept = 0;
            virtual void close () = 0;

            /// \brief Return true if the object was created as writable.
            /// \note This does not necessarily reflect the underlying file system's read/write
            /// flag: this function may return true, but a write() might still fail.
            virtual bool is_writable () const noexcept = 0;

            /// \brief Returns the name of the file originally associated with this file object.
            /// \note Depending on the host operating system, if the file was moved or deleted since
            /// it was opened, the result may no longer be accurate.
            /// \returns The name of the file originally associated with the file object.
            virtual std::string path () const = 0;

            /// \brief Sets the file position indicator for the file.
            ///
            /// The new position, measured in bytes, is given by the 'position' parameter.
            ///
            /// \param position  The file offset to be used as the file position indicator.
            virtual void seek (std::uint64_t position) = 0;

            /// \brief Obtains the current value of the position indicator for the file.
            virtual std::uint64_t tell () = 0;

            virtual std::time_t latest_time () const = 0;

            ///@{
            /// \brief Reads instances of a standard-layout type from the file.

            /// Reads a contigious series of instances of SpanType::element_type, which must be
            /// a StandardLayoutType.
            /// \param s  A span of instances which may contain zero or more members.
            /// \return The number of bytes read.
            template <typename SpanType, typename = typename std::enable_if<std::is_standard_layout<
                                             typename SpanType::element_type>::value>::type>
            std::size_t read_span (SpanType const & s) {
                auto const size = s.size_bytes ();
                assert (size >= 0);
                using utype = typename std::make_unsigned<decltype (size)>::type;
                return this->read_buffer (s.data (), static_cast<utype> (s.size_bytes ()));
            }

            /// \brief Reads a series of raw bytes from the file as an instance of type T.
            template <typename T,
                      typename = typename std::enable_if<std::is_standard_layout<T>::value>::type>
            void read (T * const t) {
                assert (t != nullptr);
                if (this->read_buffer (t, sizeof (T)) != sizeof (T)) {
                    raise (error_code::did_not_read_number_of_bytes_requested);
                }
            }
            ///@}

            ///@{
            /// \brief Writes instances of a standard-layout type to the file.
            template <typename SpanType, typename = typename std::enable_if<std::is_standard_layout<
                                             typename SpanType::element_type>::value>::type>
            void write_span (SpanType const & s) {
                auto const bytes = s.size_bytes ();
                assert (bytes >= 0);
                auto const ubytes =
                    static_cast<typename std::make_unsigned<decltype (bytes)>::type> (bytes);
                this->write_buffer (s.data (), ubytes);
            }

            /// \brief Writes 't' as a series of raw bytes to the file.
            template <typename T,
                      typename = typename std::enable_if<std::is_standard_layout<T>::value>::type>
            void write (T const & t) {
                this->write_buffer (&t, sizeof (T));
            }
            ///@}


            virtual std::uint64_t size () = 0;
            virtual void truncate (std::uint64_t size) = 0;

            /// \name File range locking
            ///
            /// Two kinds of locks are offered: shared locks and exclusive locks. Shared locks can
            /// be acquired by a number of processes at the same time, but an exclusive lock can
            /// only be acquired by one process, and cannot coexist with a shared lock. To acquire a
            /// shared lock, a process must wait until no processes hold any exclusive locks. To
            /// acquire an exclusive lock, a process must wait until no processes hold either kind
            /// of lock.
            ///
            /// Shared locks are sometimes called "read locks" and exclusive locks are sometimes
            /// called "write locks".
            ///
            ///@{

            /// \enum blocking_mode
            /// \brief Indicates whether the lock() method should block until the lock has been
            /// obtained.
            enum class blocking_mode {
                non_blocking, ///< The call will return immediately
                blocking,     ///< The call will block until the lock has been obtained
            };
            /// \enum lock_kind
            /// \brief Represents the type of file range lock to be obtained.
            enum class lock_kind {
                shared_read,     ///< Specifies a read (or shared) lock
                exclusive_write, ///< Specifies a write (or exclusive) lock
            };

            /// \brief Obtains a shared-read or exclusive-write lock on the file range specified by
            /// the 'offset' and size' parameters.
            ///
            /// If the blocking-mode is 'blocking' and another process has an exclusive_write lock
            /// on the specified range, then the call will block execution until the lock is
            /// acquired.
            ///
            /// \note lock() is usually not called directly: range_lock, wrapped with
            /// std::unique_lock<>, is used to coordinate calls to lock() and unlock().
            ///
            /// \param offset  The offset of the first byte of the file to be locked.
            /// \param size    The number of bytes to be locked.
            /// \param lt      Specifies the type of lock to be obtained
            /// \param bl      Indicates whether the function should block until the lock has been
            /// obtained or return immediately.
            /// \returns True if the lock was successfully obtained, false otherwise.
            virtual bool lock (std::uint64_t offset, std::size_t size, lock_kind lt,
                               blocking_mode bl) = 0;

            /// \brief Unlocks the file bytes specified by 'offset' and 'size'.
            ///
            /// The same file range must be locked by the current process, otherwise, the behavior
            /// is undefined.
            ///
            /// \note unlock() is usually not called directly: range_lock, wrapped with
            /// std::unique_lock<>, is used to coordinate calls to lock() and unlock().
            ///
            /// \param offset  The offset of the first byte of the file to be locked.
            /// \param size    The number of bytes to be locked.
            virtual void unlock (std::uint64_t offset, std::size_t size) = 0;
            ///@}

        protected:
            file_base () noexcept = default;

            /// \name Raw file I/O
            /// The following methods are protected because they provide the raw read/write-bytes
            /// functions. Use of the read_span(), write_span(), and write() template convenience
            /// functions is preferred.
            ///@{

            /// \brief Reads nbytes from the file, storing them at the location given by buffer.
            ///
            /// Returns the number of bytes read. The file position indicator for the file is
            /// incremented by the number of bytes read.
            ///
            /// \param buffer  A pointer to the memory which will recieve bytes read from the file.
            /// There must be at least nbytes available.
            /// \param nbytes  The number of bytes that are to be read.
            /// \returns The number of bytes actually read.
            virtual std::size_t read_buffer (gsl::not_null<void *> buffer, std::size_t nbytes) = 0;

            /// \brief Writes nbytes to the file, reading them from the location given by buffer.
            ///
            /// The file position indicator for the file is incremented by the number of bytes
            /// written.
            ///
            /// \param buffer  A pointer to the memory containing the data to be written. At least
            /// 'nbytes' must be available.
            /// \param nbytes  The number of bytes that are to be written.
            virtual void write_buffer (gsl::not_null<void const *> buffer, std::size_t nbytes) = 0;

            ///@}
        };


        //*                                _            _      *
        //*   _ __ __ _ _ __   __ _  ___  | | ___   ___| | __  *
        //*  | '__/ _` | '_ \ / _` |/ _ \ | |/ _ \ / __| |/ /  *
        //*  | | | (_| | | | | (_| |  __/ | | (_) | (__|   <   *
        //*  |_|  \__,_|_| |_|\__, |\___| |_|\___/ \___|_|\_\  *
        //*                   |___/                            *
        /// \brief A synchronization object that can be used to protect data in a file from being
        /// simultaneously accessed by multiple processes.
        ///
        /// It supports a non-recursive multiple-reader/single writer access model:
        ///
        /// - A process owns a range lock from the time that it successfully calls either lock or
        /// try_lock until it calls unlock.
        /// - When a process has an exclusive_write lock, all other processes will block (for calls
        /// to lock) or receive a false return for try_lock) if they attempt to claim ownership of
        /// the range_lock.
        /// - A calling process must not own the range_lock prior to calling lock or try_lock.
        ///
        /// The behavior of the program is undefined if a range lock is destroyed while still owned.
        /// The range_lock class satisfies all requirements of Lockable and StandardLayoutType; it
        /// is not copyable.
        ///
        /// \note range_lock is usually not accessed directly: std::unique_lock and std::lock_guard
        /// are used to manage locking in exception-safe manner. The class is a relatively thin
        /// wrapper around the file_base::lock() and file_base::unlock() methods.

        class range_lock {
        public:
            /// Default constructs an instance of range_lock. The following values are set:
            /// - file is nullptr
            /// - offset is 0
            /// - size is 0
            /// - kind is file_base::lock_kind::shared_read
            /// - is_locked will return false
            range_lock () noexcept = default;
            /// \param file    The file whose contents are to be range-locked.
            /// \param offset  The offset of the first byte of the file to be locked.
            /// \param size    The number of bytes to be locked.
            /// \param kind    Specifies the type of lock to be obtained
            range_lock (file_base * const file, std::uint64_t offset, std::size_t size,
                        file_base::lock_kind kind) noexcept;
            range_lock (range_lock const & other) = delete;
            range_lock (range_lock && other) noexcept;

            /// \brief Destroys the range lock.
            /// If the lock is owned, it will be unlocked but an error will be lost.
            ~range_lock () noexcept;

            /// Move assignment operator. Replaces the contents with those of "other" using move
            /// semantics. If prior to the call *this has an associated mutex and has acquired
            /// ownership of it, the mutex is unlocked. If this operation fails, the resulting
            /// exception is dropped.
            range_lock & operator= (range_lock && other) noexcept;
            range_lock & operator= (range_lock const &) = delete;

            /// \brief Blocks until a lock can be obtained for the current thread.
            ///   If an exception is thrown, no lock is obtained.
            /// \returns False if the lock was already owned before the call or no file is
            /// associated with this object otherwise true.
            bool lock ();

            /// \brief Attempts to acquire the lock for the current thread without blocking.
            ///   If an exception is thrown, no lock is obtained.
            /// \returns true if the lock was acquired, false otherwise.
            bool try_lock ();

            /// \brief Releases the file range lock which should be previously be locked by a call
            ///   to lock() or try_lock().
            void unlock ();

            /// \name Observers
            ///@{
            file_base * file () noexcept { return file_; }
            file_base const * file () const noexcept { return file_; }

            /// \returns The offset of the first locked byte of the file to be locked.
            std::uint64_t offset () const noexcept { return offset_; }
            /// \returns The number bytes to be locked.
            std::size_t size () const noexcept { return size_; }
            /// \returns The type of lock to be obtained when lock() is called.
            file_base::lock_kind kind () const noexcept { return kind_; }
            bool is_locked () const noexcept { return locked_; }
            ///@}

        private:
            /// The file whose contents are to be range-locked
            file_base * file_ = nullptr;
            /// The offset of the first byte of the file to be locked
            std::uint64_t offset_ = 0U;
            /// The number of bytes to be locked
            std::size_t size_ = 0U;
            /// Specifies the type of lock to be obtained
            file_base::lock_kind kind_ = file_base::lock_kind::shared_read;
            /// True if the file range has been locked
            bool locked_ = false;

            bool lock_impl (file_base::blocking_mode mode);

#ifdef PSTORE_HAVE_IS_TRIVIALLY_COPYABLE
            static_assert (
                std::is_trivially_copyable<decltype (file_)>::value,
                "file_ is not trivially copyable: use std::move() in move ctor and assign");
            static_assert (
                std::is_trivially_copyable<decltype (offset_)>::value,
                "offset_ is not trivially copyable: use std::move() in move ctor and assign");
            static_assert (
                std::is_trivially_copyable<decltype (size_)>::value,
                "size_ is not trivially copyable: use std::move() in move ctor and assign");
            static_assert (
                std::is_trivially_copyable<decltype (kind_)>::value,
                "kind_ is not trivially copyable: use std::move() in move ctor and assign");
            static_assert (
                std::is_trivially_copyable<decltype (locked_)>::value,
                "locked_ is not trivially copyable: use std::move() in move ctor and assign");
#endif // PSTORE_HAVE_IS_TRIVIALLY_COPYABLE
        };



        //*   _                                                     *
        //*  (_)_ __    _ __ ___   ___ _ __ ___   ___  _ __ _   _   *
        //*  | | '_ \  | '_ ` _ \ / _ \ '_ ` _ \ / _ \| '__| | | |  *
        //*  | | | | | | | | | | |  __/ | | | | | (_) | |  | |_| |  *
        //*  |_|_| |_| |_| |_| |_|\___|_| |_| |_|\___/|_|   \__, |  *
        //*                                                 |___/   *
        /// \brief Implements an in-memory file which provides a file-like API to a chunk of
        /// pre-allocated memory.
        class in_memory final : public file_base {
        public:
            /// The type which can be used to memory-map instances of in_memory files.
            using memory_mapper = pstore::in_memory_mapper;

            in_memory (std::shared_ptr<void> const & buffer, std::uint64_t const length,
                       std::uint64_t const eof = 0, bool const writable = true) noexcept
                    : buffer_ (std::static_pointer_cast<std::uint8_t> (buffer))
                    , length_ (length)
                    , eof_ (eof)
                    , writable_ (writable) {

                assert (buffer != nullptr);
                assert (eof <= length);
            }

            void close () override {}
            bool is_open () const noexcept override { return true; }
            bool is_writable () const noexcept override { return writable_; }
            std::string path () const override;

            void seek (std::uint64_t position) override;
            std::uint64_t tell () override { return pos_; }

            std::uint64_t size () override { return eof_; }
            void truncate (std::uint64_t size) override;
            std::time_t latest_time () const override;


            bool lock (std::uint64_t const /*offset*/, std::size_t const /*size*/,
                       lock_kind const /*lt*/, blocking_mode const /*bl*/) override {
                return true;
            }
            void unlock (std::uint64_t const /*offset*/, std::size_t const /*size*/) override {}

            /// Returns the underlying memory managed by the file object.
            std::shared_ptr<void> data () { return buffer_; }

            /// Reads nbytes from the file, storing them at the location given by buffer. Returns
            /// the number of bytes read. The file position indicator for the file is incremented by
            /// the number of bytes read.
            ///
            /// \note This member function is protected in the base class. I make it public here for
            /// unit testing.
            std::size_t read_buffer (gsl::not_null<void *> buffer, std::size_t nbytes) override;

            /// Writes writes nbytes to the file, reading them from the location given by ptr. The
            /// file position indicator for the file is incremented by the number of bytes written.
            ///
            /// \note This member function is protected in the base class. I make it public here for
            /// unit testing.
            void write_buffer (gsl::not_null<void const *> ptr, std::size_t nbytes) override;

        private:
            /// The buffer used by the in-memory file.
            std::shared_ptr<std::uint8_t> buffer_;

            /// The number of bytes in the in-memory buffer.
            std::uint64_t const length_;

            /// The number of bytes in the core_filer::buffer array that have been written and
            /// provides the simulated file size.. Always less or equal to than length_.
            std::uint64_t eof_;

            /// Is the file writable? We don't make any attempt to make the memory managed
            /// by the file physically read-only, so there's no hardware enforcement of this
            /// state.
            bool writable_;

            /// The file position indicator.
            std::uint64_t pos_ = UINT64_C (0);

            void check_writable () const;
        };


        //*    __ _ _        _                     _ _        *
        //*   / _(_) | ___  | |__   __ _ _ __   __| | | ___   *
        //*  | |_| | |/ _ \ | '_ \ / _` | '_ \ / _` | |/ _ \  *
        //*  |  _| | |  __/ | | | | (_| | | | | (_| | |  __/  *
        //*  |_| |_|_|\___| |_| |_|\__,_|_| |_|\__,_|_|\___|  *
        //*                                                   *
        /// \brief Implements a portable file access API.
        class file_handle final : public file_base {
        public:
            /// The type which can be used to memory-map instances of physcial files.
            using memory_mapper = pstore::memory_mapper;

            enum class create_mode {
                create_new,    ///< Creates a new file, only if it does not already exist
                open_existing, ///< Opens a file only if it already exists
                open_always ///< Opens an existing file if present or creates a new file otherwise
            };

            /// \enum present_mode
            /// \brief Controls the behavior of the file_handle constructor when passed a path which
            /// does not already exist.
            enum class present_mode {
                /// If opening a file that does not exist, an exception will not be raised. The
                /// condition can be detected by calling is_open() and testing for a true result. If
                /// the file was not found, then any attempt to operate on the file -- such as
                /// reading, writing, seeking, and so on -- will fail with an exception being
                /// raised.
                allow_not_found,

                /// If attempting to open a file that does not exist, an exception is raised.
                /// This mode is meaningless if used in conjunction with create_mode::create_new..
                must_exist
            };
            /// \enum writable_mode
            /// \brief Controls whether the file_handle constructor produces a read-only or
            /// read/write object.
            enum class writable_mode {
                read_only,
                read_write,
            };

            /// unique is an empty class type used to disambiguate the overloads of creating a file.
            struct unique {};
            /// temporary is an empty class type used to disambiguate the overloads of creating a
            /// file.
            struct temporary {};


            file_handle () = default;
            explicit file_handle (std::string path) noexcept
                    : path_{std::move (path)} {}
            file_handle (file_handle const &) = delete;
            file_handle (file_handle && other) noexcept;

            ~file_handle () noexcept override;

            file_handle & operator= (file_handle && rhs) noexcept;
            file_handle & operator= (file_handle const &) = delete;


            void open (create_mode create, writable_mode writable,
                       present_mode present = present_mode::allow_not_found);
            /// Create a new, uniquely named, file in the specified directory.
            void open (unique, std::string const & directory);

            /// Creates a temporary file in the system temporary directory
            void open (temporary const t) {
                return open (t, file_handle::get_temporary_directory ());
            }
            /// Creates a temporary file in the specified directory.
            void open (temporary, std::string const & directory);


            /// Returns a UTF-8 encoded string representing the temporary directory.
            ///
            /// \note The code determines the temporary directory using the Win32
            /// GetTempPathW() API which performs no validation on the path. The
            /// code needs to be prepared for an attempt to create a file in this
            /// directory to fail.
            static std::string get_temporary_directory ();

            bool is_open () const noexcept override { return file_ != invalid_oshandle; }
            bool is_writable () const noexcept override { return is_writable_; }
            std::string path () const override { return path_; }

            void close () override;

            void seek (std::uint64_t position) override;
            std::uint64_t tell () override;
            std::uint64_t size () override;
            void truncate (std::uint64_t size) override;
            /// Renames a file from one UTF-8 encoded path to another.
            /// \returns True on success, false if the rename failed because the target file already
            /// existed.
            bool rename (std::string const & new_name);

            std::time_t latest_time () const override;

            bool lock (std::uint64_t offset, std::size_t size, lock_kind kind,
                       blocking_mode block) override;
            void unlock (std::uint64_t offset, std::size_t size) override;

#ifdef _WIN32
            using oshandle = HANDLE;
            // TODO: making invalid_oshandle constexpr results in it having the value 0 (rather than
            // -1).
            static oshandle const invalid_oshandle;
#else
            using oshandle = int;
            static constexpr oshandle invalid_oshandle = -1;
#endif

            oshandle raw_handle () noexcept { return file_; }

        private:
            std::size_t read_buffer (gsl::not_null<void *> buffer, std::size_t nbytes) override;
            void write_buffer (gsl::not_null<void const *> buffer, std::size_t nbytes) override;
            void ensure_open ();
            static oshandle close (oshandle file, std::string const & path);

#ifdef _WIN32
#else
            static int lock_reg (int fd, int cmd, short type, off_t offset, short whence,
                                 off_t len);
#endif
            std::string path_ = "<unknown>";
            oshandle file_ = invalid_oshandle;
            bool is_writable_ = false;

#ifdef PSTORE_HAVE_IS_TRIVIALLY_COPYABLE
            static_assert (
                std::is_trivially_copyable<decltype (file_)>::value,
                "file_ is not trivially copyable: use std::move() in rvalue ref ctor and assign");
            static_assert (std::is_trivially_copyable<decltype (is_writable_)>::value,
                           "is_writable_ is not trivially copyable: use std::move() in rvalue ref "
                           "ctor and assign");
#endif // PSTORE_HAVE_IS_TRIVIALLY_COPYABLE
        };

        // ensure_open
        // ~~~~~~~~~~~
        inline void file_handle::ensure_open () {
            if (!this->is_open ()) {
                raise (std::errc::bad_file_descriptor);
            }
        }

        std::ostream & operator<< (std::ostream & os, file_handle const & fh);


        //*       _      _      _              _                      *
        //*    __| | ___| | ___| |_ ___ _ __  | |__   __ _ ___  ___   *
        //*   / _` |/ _ \ |/ _ \ __/ _ \ '__| | '_ \ / _` / __|/ _ \  *
        //*  | (_| |  __/ |  __/ ||  __/ |    | |_) | (_| \__ \  __/  *
        //*   \__,_|\___|_|\___|\__\___|_|    |_.__/ \__,_|___/\___|  *
        //*                                                           *
        /// \brief A class which, on destruction, will delete a file whose name is
        ///        passed to the constructor.
        /// The path can be "released" (so that it won't be deleted) by calling the release()
        /// method.
        class deleter_base {
        public:
            using unlink_proc = std::function<void (std::string const &)>;

            virtual ~deleter_base () noexcept;

            // No copying, moving, or assignment
            deleter_base (deleter_base const &) = delete;
            deleter_base (deleter_base &&) noexcept = delete;
            deleter_base & operator= (deleter_base const &) = delete;
            deleter_base & operator= (deleter_base &&) noexcept = delete;

            /// \brief Explicitly deletes the file at the path given to the
            ///        constructor.
            /// Calling this function will immediately delete the file at the path
            /// passed to the constructor unless the release() method has been
            /// called. If this function is called, the destructor will not attempt
            /// to delete the file a second time.
            void unlink ();

            /// Releases the file path given to the constructor so that it will _not_
            /// be deleted when the instance is destroyed.
            void release () noexcept { released_ = true; }

        protected:
            explicit deleter_base (std::string path, unlink_proc unlinker);

        private:
            /// The path to the file that will be deleted when the instance is
            /// destroyed or unlink() is called.
            std::string path_;

            /// The OS-specific function which is responsible for the actual deletion
            /// of the file. Also used by the unit tests to observe when files will be
            /// deleted.
            unlink_proc unlinker_;

            /// Initialized to false and set to true if release() is called. If true,
            /// the unlinker_ method will not be called.
            bool released_ = false;
        };


        /// \brief Returns true if the file system contains an object at the location given by \p
        /// path.
        /// \param path  A location in the file system to be checked.
        /// \returns true if the file system contains an object at the location given by \p path.
        /// False otherwise.
        bool exists (std::string const & path);

        /// \brief Deletes the file system object at the location given by \p path.
        /// \param path The location in the file system of the object to be deleted.
        /// \param allow_noent  If true, do not raise an error if the file did not exist.
        void unlink (std::string const & path, bool allow_noent = false);

    } // end namespace file
} // end namespace pstore

#if defined(_WIN32)
#    include "file_win32.hpp"
#else
#    include "file_posix.hpp"
#endif

#endif // PSTORE_OS_FILE_HPP
