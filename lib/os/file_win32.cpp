//*   __ _ _       *
//*  / _(_) | ___  *
//* | |_| | |/ _ \ *
//* |  _| | |  __/ *
//* |_| |_|_|\___| *
//*                *
//===- lib/os/file_win32.cpp ----------------------------------------------===//
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
/// \file file_win32.cpp
/// \brief Windows-specific implementations of file APIs.

#ifdef _WIN32

#    include "pstore/os/file.hpp"

// standard includes
#    include <algorithm>
#    include <cassert>
#    include <cstdlib>
#    include <cstring>
#    include <ctime>
#    include <iomanip>
#    include <limits>
#    include <sstream>
#    include <stdexcept>
#    include <system_error>

// pstore includes
#    include "pstore/adt/small_vector.hpp"
#    include "pstore/os/path.hpp"
#    include "pstore/os/uint64.hpp"
#    include "pstore/support/error.hpp"
#    include "pstore/support/gsl.hpp"
#    include "pstore/support/random.hpp"
#    include "pstore/support/utf.hpp"
#    include "pstore/support/quoted.hpp"

namespace pstore {
    namespace file {

        bool exists (std::string const & path) {
            std::wstring const utf16_path = pstore::utf::win32::to16 (path);

            WIN32_FIND_DATAW find_data;
            HANDLE handle = ::FindFirstFileW (utf16_path.c_str (), &find_data);
            bool found = handle != INVALID_HANDLE_VALUE;
            if (found) {
                ::FindClose (handle);
            }
            return found;
        }

    } // namespace file
} // namespace pstore


namespace {

    PSTORE_NO_RETURN void raise_file_error (std::error_code const & err,
                                            pstore::gsl::czstring const message,
                                            std::string const & path) {
        std::ostringstream str;
        str << message << ' ' << pstore::quoted (path);
        pstore::raise_error_code (err, str.str ());
    }

    PSTORE_NO_RETURN void raise_file_error (DWORD const err, pstore::gsl::czstring const message,
                                            std::string const & path) {
        std::ostringstream str;
        str << message << ' ' << pstore::quoted (path);
        raise (pstore::win32_erc{err}, str.str ());
    }

    HANDLE create_new_file (std::string const & path, bool is_temporary) {

        // A "creation disposition" of CREATE_NEW means that if the specified
        // file exists, the function fails and the last-error code is set to
        // ERROR_FILE_EXISTS.
        DWORD const creation_disposition = CREATE_NEW;

        // Tell the file system that this is a temporary file.
        DWORD const flags = is_temporary ? (FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE)
                                         : FILE_ATTRIBUTE_NORMAL;

        HANDLE const handle =
            ::CreateFileW (pstore::utf::win32::to16 (path).c_str (), // "file name"
                           GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, // "security attributes"
                           creation_disposition,
                           flags,    // "flags and attributes"
                           nullptr); // "template file"
        if (handle == INVALID_HANDLE_VALUE) {
            DWORD const last_error = ::GetLastError ();
            if (last_error != ERROR_FILE_EXISTS) {
                raise_file_error (last_error, "Could not open temporary file", path);
            }
        }
        return handle;
    }

    // mkstemp
    // ~~~~~~~
    /// \brief The mktemp() function takes a file name template and creates the template file.
    /// This function guarantees that the file is created atomically
    /// \param tmpl
    /// \param is_temporary
    /// \return

    std::tuple<HANDLE, std::string> mkstemp (std::string const & tmpl, bool is_temporary) {
        using pstore::file::details::name_from_template;
        static pstore::random_generator<unsigned> rng;

        unsigned tries = 10;
        while (tries-- > 0) {
            std::string const path =
                name_from_template (tmpl, [] (unsigned max) { return rng.get (max); });
            HANDLE const handle = create_new_file (path, is_temporary);
            // create_new_file() returns INVALID_HANDLE_VALUE if the file exists;
            // we dream up a new name and try again.
            if (handle != INVALID_HANDLE_VALUE) {
                return std::make_tuple (handle, path);
            }
        }

        pstore::raise (std::errc::file_exists);
    }


    // file_time_to_epoch
    // ~~~~~~~~~~~~~~~~~~
    std::time_t file_time_to_epoch (std::uint64_t ticks) {
        // The number of days between Jan 1 1601 (the FILETIME 0 day) and
        // Jan 1 1970 (the Epoch zero day).
        constexpr auto days_to_epoch = std::uint64_t{134774};

        constexpr auto seconds_to_epoch = days_to_epoch * 24 * 60 * 60;
        constexpr auto ticks_per_second = std::uint64_t{10000000};
        return ticks / ticks_per_second - seconds_to_epoch;
    }

    std::time_t file_time_to_epoch (FILETIME const & file_time) {
        return file_time_to_epoch ((static_cast<std::uint64_t> (file_time.dwHighDateTime) << 32) |
                                   file_time.dwLowDateTime);
    }
} // namespace


namespace pstore {
    namespace file {
        file_handle::oshandle const file_handle::invalid_oshandle = INVALID_HANDLE_VALUE;

        // open
        // ~~~~
        void file_handle::open (create_mode create, writable_mode writable, present_mode present) {
            this->close ();
            is_writable_ = writable == writable_mode::read_write;

            DWORD desired_access = GENERIC_READ;
            if (is_writable_) {
                desired_access |= GENERIC_WRITE;
            }

            DWORD creation_disposition = 0;
            switch (create) {
            // Creates a new file, only if it does not already exist
            case create_mode::create_new: creation_disposition = CREATE_NEW; break;
            // Opens a file only if it already exists
            case create_mode::open_existing: creation_disposition = OPEN_EXISTING; break;
            // Opens an existing file if present, and creates a new file otherwise.
            case create_mode::open_always: creation_disposition = OPEN_ALWAYS; break;
            }

            file_ = ::CreateFileW (pstore::utf::win32::to16 (path_).c_str (), // "file name"
                                   desired_access,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   nullptr, // "security attributes"
                                   creation_disposition,
                                   FILE_ATTRIBUTE_NORMAL, // "flags and attributes"
                                   nullptr);              // "template file"
            if (file_ == invalid_oshandle) {
                DWORD const last_error = ::GetLastError ();
                if (present == present_mode::allow_not_found &&
                    last_error == ERROR_FILE_NOT_FOUND) {
                    file_ = invalid_oshandle;
                } else {
                    raise_file_error (last_error, "Unable to open", path_);
                }
            }
        }

        void file_handle::open (temporary, std::string const & directory) {
            this->close ();
            is_writable_ = true;

            // mkstemp() returns the actual name of the temporary file that was created
            // as well as the open handle.
            std::tie (file_, path_) =
                mkstemp (path::join (directory, "pst-XXXXXX"), true /*is_temporary*/);
        }

        /// Create a new, uniquely named, file in the specified directory.
        void file_handle::open (unique, std::string const & directory) {
            this->close ();
            is_writable_ = true;

            // mkstemp() returns the actual name of the temporary file that was created
            // as well as the open handle.
            //
            // We set the is_temporary parameter to false so that we don't create
            // it as a Windows temporary file (which is deleted on close).
            std::tie (file_, path_) =
                mkstemp (path::join (directory, "pst-XXXXXX"), false /*is_temporary*/);
        }

        // close
        // ~~~~~
        void file_handle::close () {
            is_writable_ = false;
            auto file_or_error = file_handle::close_noex (file_);
            if (!file_or_error) {
                raise_file_error (file_or_error.get_error (), "Unable to close", path_);
            }
            file_ = *file_or_error;
        }

        // close_noex
        // ~~~~~~~~~~
        auto file_handle::close_noex (oshandle const file) -> error_or<oshandle> {
            if (file != invalid_oshandle && !::CloseHandle (file)) {
                return error_or<oshandle>{make_error_code (pstore::win32_erc (::GetLastError ()))};
            }
            return {in_place, invalid_oshandle};
        }

        // seek
        // ~~~~
        void file_handle::seek (std::uint64_t position) {
            this->ensure_open ();

            // Microsoft's LARGE_INTEGER type is signed.
            using quad_part_type = decltype (LARGE_INTEGER::QuadPart);
            static_assert (sizeof (quad_part_type) == sizeof (LARGE_INTEGER),
                           "QuadPart may not be the largest member of LARGE_INTEGER");
            assert (position < static_cast<std::make_unsigned<quad_part_type>::type> (
                                   std::numeric_limits<quad_part_type>::max ()));

            LARGE_INTEGER distance;
            distance.QuadPart = static_cast<quad_part_type> (position);
            if (!::SetFilePointerEx (file_, distance, static_cast<LARGE_INTEGER *> (nullptr),
                                     FILE_BEGIN)) {
                DWORD const last_error = ::GetLastError ();
                raise_file_error (last_error, "Unable to set file position", path_);
            }
        }

        // tell
        // ~~~~
        std::uint64_t file_handle::tell () {
            this->ensure_open ();
            auto old_pos = LARGE_INTEGER{0};
            auto new_pos = LARGE_INTEGER{0};
            if (!::SetFilePointerEx (file_, old_pos, &new_pos, FILE_CURRENT)) {
                DWORD const last_error = ::GetLastError ();
                raise_file_error (last_error, "Unable to get file position", path_);
            }
            return new_pos.QuadPart;
        }

        // read
        // ~~~~
        std::size_t file_handle::read_buffer (gsl::not_null<void *> buffer, std::size_t size) {
            this->ensure_open ();

            return details::split<DWORD> (
                buffer, size, [this](void * ptr, DWORD num_to_read) -> DWORD {
                    auto num_read = DWORD{0};
                    if (!::ReadFile (file_, ptr, num_to_read, &num_read, nullptr /*overlapped*/)) {
                        DWORD const last_error = ::GetLastError ();
                        raise_file_error (last_error, "Unable to read", path_);
                    }
                    return num_read;
                });
        }

        // write_buffer
        // ~~~~~~~~~~~~
        void file_handle::write_buffer (gsl::not_null<void const *> buffer, std::size_t size) {
            this->ensure_open ();

            std::size_t const bytes_written = details::split<DWORD> (
                buffer, size, [this](std::uint8_t const * ptr, DWORD num_to_write) -> DWORD {
                    auto num_written = DWORD{0};
                    if (!::WriteFile (file_, ptr, num_to_write, &num_written,
                                      nullptr /* not overlapped*/)) {
                        DWORD const last_error = ::GetLastError ();
                        raise_file_error (last_error, "Unable to write", path_);
                    }
                    return num_written;
                });
            if (bytes_written != size) {
                raise (std::errc::invalid_argument,
                       "Didn't write the number of bytes that were requested");
            }
            assert (is_writable_);
        }

        // size
        // ~~~~
        std::uint64_t file_handle::size () {
            this->ensure_open ();

            LARGE_INTEGER result{};
            if (!::GetFileSizeEx (file_, &result)) {
                DWORD const last_error = ::GetLastError ();
                raise_file_error (last_error, "Unable to get size of file", path_);
            }
            return result.QuadPart;
        }

        // truncate
        // ~~~~~~~~
        void file_handle::truncate (std::uint64_t size) {
            this->ensure_open ();

            auto const old_pos = this->tell ();
            this->seek (size);
            if (!::SetEndOfFile (file_)) {
                auto const set_eof_error_code = ::GetLastError ();
                raise_file_error (set_eof_error_code, "Unable to set file size of", path_);
            }

            this->seek (std::min (size, old_pos));
        }

        // rename
        // ~~~~~~
        bool file_handle::rename (std::string const & new_name) {
            bool result = true;
            // Deliberately do not pass the MOVEFILE_COPY_ALLOWED to MoveFileExW() because this
            // could mean that the copy is anything but atomic.
            if (!::MoveFileExW (pstore::utf::win32::to16 (path_).c_str (),    // original file name
                                pstore::utf::win32::to16 (new_name).c_str (), // new name
                                0)) {                                         // flags
                auto const last_error = ::GetLastError ();
                if (last_error == ERROR_ALREADY_EXISTS) {
                    result = false;
                } else {
                    std::ostringstream message;
                    message << "Unable to rename " << pstore::quoted (path_) << " to "
                            << pstore::quoted (new_name);
                    raise (win32_erc{last_error}, message.str ());
                }
            }
            path_ = new_name;
            return result;
        }

        // lock
        // ~~~~
        bool file_handle::lock (std::uint64_t offset, std::size_t size, lock_kind kind,
                                blocking_mode block) {
            this->ensure_open ();

            OVERLAPPED overlapped{};
            overlapped.Offset = uint64_low4 (offset);
            overlapped.OffsetHigh = uint64_high4 (offset);

            auto flags = DWORD{0};
            switch (block) {
            case blocking_mode::non_blocking: flags |= LOCKFILE_FAIL_IMMEDIATELY; break;
            case blocking_mode::blocking: break;
            }
            switch (kind) {
            case lock_kind::exclusive_write: flags |= LOCKFILE_EXCLUSIVE_LOCK; break;
            case lock_kind::shared_read: break;
            }
            bool got_lock = true;
            if (!::LockFileEx (file_, flags,
                               0,                   // "reserved, must be 0"
                               uint64_low4 (size),  // number of bytes to lock (low)
                               uint64_high4 (size), // number of bytes to lock (high)
                               &overlapped)         // "overlapped"
            ) {
                // If the LOCKFILE_FAIL_IMMEDIATELY flag is specified and an exclusive lock is
                // requested for a range of a file that already has a shared or exclusive lock, the
                // function returns the error ERROR_IO_PENDING.
                DWORD const last_error = ::GetLastError ();
                if (block == blocking_mode::non_blocking && last_error == ERROR_IO_PENDING) {
                    got_lock = false;
                } else {
                    raise_file_error (last_error, "Unable to lock range of", path_);
                }
            }
            return got_lock;
        }

        // unlock
        // ~~~~~~
        void file_handle::unlock (std::uint64_t offset, std::size_t size) {
            this->ensure_open ();

            OVERLAPPED overlapped{};
            overlapped.Offset = uint64_low4 (offset);
            overlapped.OffsetHigh = uint64_high4 (offset);
            if (!::UnlockFileEx (file_,
                                 0,                   // "reserved, must be 0"
                                 uint64_low4 (size),  // number of bytes to unlock (low)
                                 uint64_high4 (size), // number of bytes to unlock (high)
                                 &overlapped)) {      // "overlapped"
                DWORD const last_error = ::GetLastError ();
                raise_file_error (last_error, "Unable to unlock range of", path_);
            }
        }

        // latest_time
        // ~~~~~~~~~~~
        time_t file_handle::latest_time () const {
            file_handle local_file{path_};
            HANDLE h = file_;
            if (!this->is_open ()) {
                local_file.open (create_mode::open_existing, writable_mode::read_only,
                                 present_mode::must_exist);
                h = local_file.file_;
            }

            FILETIME creation_time;
            FILETIME last_access_time;
            FILETIME last_write_time;
            if (!::GetFileTime (h, &creation_time, &last_access_time, &last_write_time)) {
                DWORD const last_error = ::GetLastError ();
                raise_file_error (last_error, "Unable to get file time for", path_);
            }

            return file_time_to_epoch (
                std::max ({creation_time, last_access_time, last_write_time},
                          [](FILETIME const & lhs, FILETIME const & rhs) {
                              return std::make_pair (lhs.dwHighDateTime, lhs.dwLowDateTime) <
                                     std::make_pair (rhs.dwHighDateTime, rhs.dwLowDateTime);
                          }));
        }

        // get_temporary_directory [static]
        // ~~~~~~~~~~~~~~~~~~~~~~~
        std::string file_handle::get_temporary_directory () {
            small_vector<wchar_t> temp_path;
            temp_path.resize (temp_path.capacity ());
            DWORD num_code_units =
                ::GetTempPathW (static_cast<DWORD> (temp_path.size ()), temp_path.data ());
            if (num_code_units == 0) {
                DWORD const last_error = ::GetLastError ();
                raise (win32_erc{last_error}, "GetTempPathW");
            } else if (num_code_units > temp_path.size ()) {
                // Increase the buffer size and try again.
                temp_path.resize (num_code_units);
                num_code_units =
                    ::GetTempPathW (static_cast<DWORD> (temp_path.size ()), temp_path.data ());
                if (num_code_units == 0) {
                    DWORD const last_error = ::GetLastError ();
                    raise (win32_erc{last_error}, "GetTempPathW");
                } else if (num_code_units != temp_path.size ()) {
                    raise (std::errc::no_buffer_space, "GetTempPathW");
                }
            }

            return utf::win32::to8 (temp_path.data (), num_code_units);
        }

        namespace win32 {

            // An out-of-line virtual destructor to avoid a vtable in every file that includes
            // deleter.
            deleter::~deleter () noexcept = default;

            void deleter::platform_unlink (std::string const & path) { file::unlink (path, true); }

        } // namespace win32

        void unlink (std::string const & path, bool allow_noent) {
            // Delete the file at the path given by the (UTF-8-encoded) parameter string.
            if (::DeleteFileW (utf::win32::to16 (path).c_str ()) == 0) {
                DWORD const last_error = ::GetLastError ();
                if (!allow_noent || last_error != ERROR_FILE_NOT_FOUND) {
                    raise_file_error (last_error, "Unable to delete", path);
                }
            }
        }

    } // end namespace file
} // end namespace pstore
#endif // #ifdef _WIN32
