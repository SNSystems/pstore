//*      _                        _                                              *
//*  ___| |__   __ _ _ __ ___  __| |  _ __ ___   ___ _ __ ___   ___  _ __ _   _  *
//* / __| '_ \ / _` | '__/ _ \/ _` | | '_ ` _ \ / _ \ '_ ` _ \ / _ \| '__| | | | *
//* \__ \ | | | (_| | | |  __/ (_| | | | | | | |  __/ | | | | | (_) | |  | |_| | *
//* |___/_| |_|\__,_|_|  \___|\__,_| |_| |_| |_|\___|_| |_| |_|\___/|_|   \__, | *
//*                                                                       |___/  *
//===- include/pstore/shared_memory.hpp -----------------------------------===//
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
/// \file shared_memory.hpp

#ifndef PSTORE_SHARED_MEMORY_HPP
#define PSTORE_SHARED_MEMORY_HPP (1)

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "pstore_support/utf.hpp"
#else // !_WIN32
#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#endif // !_WIN32

#include "pstore_support/error.hpp"
#include "pstore_support/gsl.hpp"
#include "pstore_support/portab.hpp"
#include "pstore_support/quoted_string.hpp"

namespace pstore {
    namespace posix {

        // shm_name
        // ~~~~~~~~~~
        ///@{
        /// Used to build a name beginning with a slash character for a POSIX shared memory object.
        /// Following the initial slash, as many characters as possible from 'name' are copied to
        /// the output array (arr) followed by a terminating null.
        ///
        /// \param name The name to be copied
        /// \param arr  An array into which the characters are copied.
        /// \return A pointer to the output null-terminated string. Equivalent to arr.data ().
        template <typename SpanType>
        auto shm_name (std::string const & name, SpanType arr) -> ::pstore::gsl::zstring {
            using difference_type =
                std::iterator_traits<std::string::const_iterator>::difference_type;
#ifndef NDEBUG
            using udifference_type = std::make_unsigned<difference_type>::type;
#endif
            static_assert (SpanType::extent >= 2,
                           "The posix_mutex_name span must be fixed size and able to hold "
                           "at least 2 characters");
            auto out = arr.begin ();
            *(out++) = '/';

            auto name_begin = std::begin (name);
            auto name_end = name_begin;
            assert (name.length () <=
                    static_cast<udifference_type> (std::numeric_limits<difference_type>::max ()));
            auto const name_length = static_cast<difference_type> (name.length ());
            std::advance (name_end, std::min (SpanType::extent - 2, name_length));
            out = std::copy (name_begin, name_end, out);
            *out = '\0';
            return arr.data ();
        }

        template <std::size_t N>
        auto shm_name (std::string const & name, std::array<char, N> & arr)
            -> ::pstore::gsl::zstring {

            static_assert (N <= std::numeric_limits<std::ptrdiff_t>::max (),
                           "array size must not exected ptrdiff_t max");
            return shm_name (name, ::pstore::gsl::make_span (arr));
        }
        ///@}

    } // namespace posix
} // namespace pstore


namespace pstore {
    /// \brief Opens a shared memory object containing type Ty with the given name.
    template <typename Ty>
    class shared_memory {
    public:
        shared_memory ();
        explicit shared_memory (std::string const & name);
        shared_memory (shared_memory const &) = delete;
        shared_memory (shared_memory && rhs) noexcept;

        ~shared_memory ();

        shared_memory & operator= (shared_memory const &) = delete;
        shared_memory & operator= (shared_memory && rhs) noexcept;

        Ty & operator* () { return *get (); }
        Ty const & operator* () const { return *get (); }
        Ty * operator-> () { return get (); }
        Ty const * operator-> () const { return get (); }

        Ty * get () {
            auto p = ptr_.get ();
            return (p != nullptr) ? &p->contents : nullptr;
        }
        Ty const * get () const {
            auto p = ptr_.get ();
            return (p != nullptr) ? &p->contents : nullptr;
        }

    private:
        /// \brief A simple spin-lock mutex implementation built on std::atomic_flag.
        class spin_lock {
        public:
            /// Constructs the mutex. The mutex is in unlocked state after the constructor
            /// completes.
            explicit spin_lock (::pstore::gsl::not_null<std::atomic_flag *> lock)
                    : lock_ (lock) {}

            // No copying or assignment.
            spin_lock (spin_lock const &) = delete;
            spin_lock & operator= (spin_lock const &) = delete;

            /// Locks the mutex. If another thread has already locked the mutex, a call to lock will
            /// block execution until the lock is acquired. Prior unlock() operations on the same
            /// mutex synchronize-with (as defined in std::memory_order) this operation.
            void lock () noexcept {
                while (lock_->test_and_set (std::memory_order_acquire)) {
                    // Spin until acquired
                }
            }

            /// Unlocks the mutex. The mutex must be locked by the current thread of execution,
            /// otherwise, the behavior is undefined. This operation synchronizes-with (as defined
            /// in std::memory_order) any subsequent lock operation that obtains ownership of the
            /// same mutex.
            void unlock () noexcept {
                lock_->clear (std::memory_order_release); // Release lock
            }

        private:
            std::atomic_flag * const lock_;
        };

        struct value_type {
            /// This atomic value is used for a spinlock whose sole purpose in life
            /// it to guard the initialization of the Ty instance contained wihtin this struct.
            std::atomic_flag lock{false};

            /// This value indicates whether the Ty instance in this struct has been
            /// initialized. This field must only be accessed whilst holding the spin-lock
            /// controlled by 'lock'.
            std::atomic_flag init_done{false};

            Ty contents;
        };
        static_assert (std::is_standard_layout<value_type>::value,
                       "value_type must be StandardLayout");


#ifdef _WIN32
        typedef HANDLE os_file_handle;
        // FIXME: these were copied from memory_mapper_win32.cpp
        static constexpr unsigned dword_bits = sizeof (DWORD) * 8;
        static inline constexpr auto high4 (std::uint64_t v) -> DWORD {
            return static_cast<DWORD> (v >> dword_bits);
        }
        static inline constexpr auto low4 (std::uint64_t v) -> DWORD {
            return v & ((std::uint64_t{1} << dword_bits) - 1U);
        }
#else
        typedef int os_file_handle;
#endif

        class file_mapping {
        public:
            explicit file_mapping (char const * name)
                    : descriptor_ (open (name)) {}
            ~file_mapping () noexcept;
            void set_size ();
            os_file_handle get () { return descriptor_; }

        private:
            static os_file_handle open (char const * name);
            os_file_handle descriptor_;
        };

        using pointer_type = std::unique_ptr<value_type, void (*) (value_type *)>;
        auto mmap (os_file_handle map_file) -> pointer_type;
        /// unique_ptr deleter
        static void unmap (value_type * p);

        struct shm_name {
        public:
            shm_name () = default;
            explicit shm_name (std::string const & name);
            shm_name (shm_name && rhs) noexcept = default;
            shm_name (shm_name const & rhs) = delete;

            shm_name & operator= (shm_name const & rhs) = delete;
            shm_name & operator= (shm_name && rhs) noexcept = default;

            char const * c_str () const;
            bool empty () const;

        private:
            std::string name_;
        };
        shm_name name_;
        pointer_type ptr_;
    };

    /// A function which returns the maximum length of a shared memory object name.
    std::size_t get_pshmnamlen ();

//***************************
//* shared_memory::shm_name *
//***************************
// (ctor)
// ~~~~~~
#ifdef _WIN32
    template <typename Ty>
    shared_memory<Ty>::shared_memory::shm_name::shm_name (std::string const & name)
            : name_ (name) {
        // Can't rely on being able to create objects in the global namespace. This requires
        // "SE_CREATE_GLOBAL_NAME" privileges, which is disabled by default for administrators,
        // services, and the local system account.
        // std::string const object_name = "Global\\" + name;
        std::replace (name_.begin (), name_.end (), '\\', '/');
    }
#else
    template <typename Ty>
    shared_memory<Ty>::shared_memory::shm_name::shm_name (std::string const & name) {
        std::size_t const len = get_pshmnamlen ();
        name_.reserve (len);
        name_ = '/';
        std::copy_n (std::begin (name), std::min (name.length (), len - 1),
                     std::back_inserter (name_));
    }
#endif
    // c_str
    // ~~~~~
    template <typename Ty>
    auto shared_memory<Ty>::shared_memory::shm_name::c_str () const -> char const * {
        return name_.c_str ();
    }
    // empty
    // ~~~~~
    template <typename Ty>
    auto shared_memory<Ty>::shared_memory::shm_name::empty () const -> bool {
        return name_.empty ();
    }


    //*****************
    //* shared_memory *
    //*****************
    // (ctor)
    // ~~~~~~
    template <typename Ty>
    shared_memory<Ty>::shared_memory ()
            : name_ ()
            , ptr_ (nullptr, &unmap) {}

    template <typename Ty>
    shared_memory<Ty>::shared_memory (std::string const & name)
            : name_ (name)
            , ptr_ (nullptr, &unmap) {

        file_mapping mapping (name_.c_str ());
        ptr_ = mmap (mapping.get ());

        // The initialization of 'contents' is guarded by a simple atomic spin-lock mutex. We MUST
        // NOT crash whilst holding this
        // mutex or we'll hang the next time through here.
        spin_lock sl (&ptr_->lock);
        std::lock_guard<spin_lock> lock (sl);

        if (!ptr_->init_done.test_and_set ()) {
            static_assert (sizeof (ptr_->contents) == sizeof (Ty),
                           "placement new buffer was not the expected size");
            new (&ptr_->contents) Ty;
        }
    }

    template <typename Ty>
    shared_memory<Ty>::shared_memory (shared_memory && rhs) noexcept
            : name_ (std::move (rhs.name_))
            , ptr_ (std::move (rhs.ptr_)) {}

    // (dtor)
    // ~~~~~~
    template <typename Ty>
    shared_memory<Ty>::~shared_memory () {
#ifndef _WIN32
        if (!name_.empty ()) {
            ::shm_unlink (name_.c_str ());
        }
#endif
    }

    // operator=
    // ~~~~~~~~~
    template <typename Ty>
    auto shared_memory<Ty>::operator= (shared_memory && rhs) noexcept -> shared_memory & {
        if (this != &rhs) {
            name_ = std::move (rhs.name_);
            ptr_ = std::move (rhs.ptr_);
        }
        return *this;
    }

#ifdef _WIN32

    // unmap
    // ~~~~~
    template <typename Ty>
    void shared_memory<Ty>::unmap (value_type * p) {
        if (::UnmapViewOfFile (p) == 0) {
            auto const error = ::GetLastError ();
            raise (win32_erc (error), "UnmapViewOfFile");
        }
    }

    // mmap
    // ~~~~
    template <typename Ty>
    auto shared_memory<Ty>::mmap (os_file_handle map_file) -> pointer_type {
        auto mapped_ptr = static_cast<value_type *> (::MapViewOfFile (map_file, FILE_MAP_ALL_ACCESS,
                                                                      0, // file offset (high)
                                                                      0, // file offset (low)
                                                                      sizeof (Ty)));
        if (mapped_ptr == nullptr) {
            auto const error = ::GetLastError ();
            raise (win32_erc (error), "MapViewOfFile");
        }
        return {mapped_ptr, &unmap};
    }

#else //!_WIN32

    // unmap
    // ~~~~~
    template <typename Ty>
    void shared_memory<Ty>::unmap (value_type * p) {
        if (::munmap (p, sizeof (Ty)) == -1) {
            raise (errno_erc{errno}, "munmap");
        }
    }

    // mmap
    // ~~~~
    template <typename Ty>
    auto shared_memory<Ty>::mmap (os_file_handle fd) -> pointer_type {
        auto ptr = static_cast<value_type *> (
            ::mmap (nullptr, sizeof (Ty), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
        if (ptr == MAP_FAILED) {
            raise (errno_erc{errno}, "mmap");
        }
        return {ptr, &unmap};
    }

#endif //!_WIN32


    //***********************************
    //* shared_memory<Ty>::file_mapping *
    //***********************************

#ifdef _WIN32

    // (dtor)
    // ~~~~~~
    template <typename Ty>
    shared_memory<Ty>::file_mapping::~file_mapping () noexcept {
        ::CloseHandle (descriptor_);
        descriptor_ = nullptr;
    }

    // open
    // ~~~~
    template <typename Ty>
    auto shared_memory<Ty>::file_mapping::open (char const * name) -> os_file_handle {

        HANDLE map_file =
            ::CreateFileMappingW (INVALID_HANDLE_VALUE, // use paging file
                                  nullptr,              // default security
                                  PAGE_READWRITE,       // read/write access
                                  high4 (sizeof (Ty)),  // maximum object size (high-order DWORD)
                                  low4 (sizeof (Ty)),   // maximum object size (low-order DWORD)
                                  utf::win32::to16 (name).c_str ()); // name of mapping object
        if (map_file == nullptr) {
            std::ostringstream str;
            str << "Couldn't create a file mapping for " << ::pstore::quoted (name);
            raise (win32_erc (::GetLastError ()), str.str ());
        }
        return map_file;
    }

#else // !_WIN32

    // (dtor)
    // ~~~~~~
    template <typename Ty>
    shared_memory<Ty>::file_mapping::~file_mapping () noexcept {
        ::close (descriptor_);
        descriptor_ = -1;
    }

    // open
    // ~~~~
    template <typename Ty>
    auto shared_memory<Ty>::file_mapping::open (char const * name) -> os_file_handle {
        int const fd = ::shm_open (name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            int const error = errno;
            if (error == ENAMETOOLONG) {
                std::ostringstream str;
                str << "shared memory object name (" << name << ") is too long";
                raise (errno_erc{error}, str.str ());
            } else {
                raise (errno_erc{error}, "shm_open");
            }
        }

        // If the shared memory object doesn't have room for at least sizeof(Ty) bytes,
        // then we need to grow it before the memory map operation.
        struct stat st;
        if (::fstat (fd, &st) == -1) {
            raise (errno_erc{errno}, "fstat");
        }
        if (st.st_size < static_cast<off_t> (sizeof (Ty))) {
            if (::ftruncate (fd, sizeof (Ty)) == -1) {
                raise (errno_erc{errno}, "ftruncate");
            }
        }
        return fd;
    }

#endif // !_WIN32
} // namespace pstore
#endif // PSTORE_SHARED_MEMORY_HPP
// eof: include/pstore/shared_memory.hpp
