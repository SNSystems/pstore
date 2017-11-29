//*   __ _ _       *
//*  / _(_) | ___  *
//* | |_| | |/ _ \ *
//* |  _| | |  __/ *
//* |_| |_|_|\___| *
//*                *
//===- lib/pstore_support/file_win32.cpp ----------------------------------===//
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
/// \file file_win32.cpp
/// \brief Windows-specific implementations of file APIs.

#ifdef _WIN32

#include "pstore_support/file.hpp"

// standard includes
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <system_error>

#include "pstore_support/error.hpp"
#include "pstore_support/random.hpp"
#include "pstore_support/small_vector.hpp"
#include "pstore_support/utf.hpp"
#include "pstore_support/path.hpp"
#include "pstore_support/quoted_string.hpp"

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
    // FIXME: this was copied from memory_mapper_win32.cpp
    constexpr unsigned dword_bits = sizeof (DWORD) * 8;
    inline constexpr auto high4 (std::uint64_t v) -> DWORD {
        return static_cast<DWORD> (v >> dword_bits);
    }
    inline constexpr auto low4 (std::uint64_t v) -> DWORD {
        return v & ((std::uint64_t{1} << dword_bits) - 1U);
    }

    HANDLE create_new_file (std::string const & path, bool is_temporary) {

        // A "creation disposition" of CREATE_NEW means that if the specified
        // file exists, the function fails and the last-error code is set to
        // ERROR_FILE_EXISTS.
        DWORD const creation_disposition = CREATE_NEW;

        // Tell the file system that this is a temporary file.
        DWORD const flags = is_temporary ? (FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE)
                                         : FILE_ATTRIBUTE_NORMAL;

        HANDLE handle =
            ::CreateFileW (pstore::utf::win32::to16 (path).c_str (), // "file name"
                           GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, // "security attributes"
                           creation_disposition,
                           flags,    // "flags and attributes"
                           nullptr); // "template file"
        if (handle == INVALID_HANDLE_VALUE) {
            DWORD const last_error = ::GetLastError ();
            if (last_error != ERROR_FILE_EXISTS) {
                std::ostringstream str;
                str << "Could not open temporary file " << pstore::quoted (path);
                raise (pstore::win32_erc (last_error), str.str ());
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
                name_from_template (tmpl, [](unsigned max) { return rng.get (max); });
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
} // (anonymous namespace)


namespace pstore {
    namespace file {
        // open
        // ~~~~
        void file_handle::open (std::string const & path, create_mode create,
                                writable_mode writable, present_mode present) {
            this->close ();
            path_ = path;
            is_writable_ = writable == writable_mode::read_write;

            DWORD desired_access = GENERIC_READ;
            if (is_writable_) {
                desired_access |= GENERIC_WRITE;
            }

            DWORD creation_disposition = 0;
            switch (create) {
            // Creates a new file, only if it does not already exist
            case create_mode::create_new:
                creation_disposition = CREATE_NEW;
                break;
            // Opens a file only if it already exists
            case create_mode::open_existing:
                creation_disposition = OPEN_EXISTING;
                break;
            // Opens an existing file if present, and creates a new file otherwise.
            case create_mode::open_always:
                creation_disposition = OPEN_ALWAYS;
                break;
            }

            file_ = ::CreateFileW (pstore::utf::win32::to16 (path).c_str (), // "file name"
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
                    std::ostringstream str;
                    str << "Unable to open " << pstore::quoted (path_);
                    raise (::pstore::win32_erc (last_error), str.str ());
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
            if (file_ != invalid_oshandle) {
                BOOL ok = ::CloseHandle (file_);
                // At least pretend that we successfully closed the file. I
                // don't want to inadvertently end up thinking the file should
                // be closed.
                file_ = invalid_oshandle;
                if (!ok) {
                    DWORD const last_error = ::GetLastError ();
                    std::ostringstream str;
                    str << "Unable to close " << pstore::quoted (path_);
                    ::pstore::raise (::pstore::win32_erc (last_error), str.str ());
                }
            }

            is_writable_ = false;
        }

        // seek
        // ~~~~
        void file_handle::seek (std::uint64_t position) {
            this->ensure_open ();

            // Microsoft's LARGE_INTEGER type is signed.
            typedef decltype (LARGE_INTEGER::QuadPart) quad_part_type;
            static_assert (sizeof (quad_part_type) == sizeof (LARGE_INTEGER),
                           "QuadPart may not be the largest member of LARGE_INTEGER");
            assert (position < static_cast<std::make_unsigned<quad_part_type>::type> (
                                   std::numeric_limits<quad_part_type>::max ()));

            LARGE_INTEGER distance;
            distance.QuadPart = static_cast<quad_part_type> (position);
            BOOL ok = ::SetFilePointerEx (file_, distance, static_cast<LARGE_INTEGER *> (nullptr),
                                          FILE_BEGIN);
            if (!ok) {
                DWORD const last_error = ::GetLastError ();
                std::ostringstream str;
                str << "Unable to seek to " << position << " in file " << pstore::quoted (path_);
                raise (::pstore::win32_erc (last_error), str.str ());
            }
        }

        // tell
        // ~~~~
        std::uint64_t file_handle::tell () {
            this->ensure_open ();
            auto old_pos = LARGE_INTEGER{0};
            auto new_pos = LARGE_INTEGER{0};
            BOOL ok = ::SetFilePointerEx (file_, old_pos, &new_pos, FILE_CURRENT);
            if (!ok) {
                DWORD const last_error = ::GetLastError ();
                std::ostringstream str;
                str << "Unable to get position of file " << pstore::quoted (path_);
                raise (::pstore::win32_erc (last_error), str.str ());
            }
            return new_pos.QuadPart;
        }

        // read
        // ~~~~
        std::size_t file_handle::read_buffer (::pstore::gsl::not_null<void *> buffer,
                                              std::size_t size) {
            this->ensure_open ();

            auto reader = [this](void * ptr, DWORD num_to_read) -> DWORD {
                auto num_read = DWORD{0};
                BOOL ok = ::ReadFile (file_, ptr, num_to_read, &num_read, nullptr /*overlapped*/);
                if (!ok) {
                    DWORD const last_error = ::GetLastError ();
                    std::ostringstream str;
                    str << "Unable to read " << pstore::quoted (path_);
                    raise (::pstore::win32_erc (last_error), str.str ());
                }
                return num_read;
            };

            return details::split<DWORD> (buffer, size, reader);
        }

        // write_buffer
        // ~~~~~~~~~~~~
        void file_handle::write_buffer (::pstore::gsl::not_null<void const *> buffer,
                                        std::size_t size) {
            this->ensure_open ();

            auto writer = [this](std::uint8_t const * ptr, DWORD num_to_write) -> DWORD {
                auto num_written = DWORD{0};
                BOOL ok = ::WriteFile (file_, ptr, num_to_write, &num_written,
                                       nullptr /* not overlapped*/);
                if (!ok) {
                    DWORD const last_error = ::GetLastError ();
                    std::ostringstream str;
                    str << "Unable to write " << pstore::quoted (path_);
                    raise (::pstore::win32_erc (last_error), str.str ());
                }
                return num_written;
            };

            std::size_t const bytes_written = details::split<DWORD> (buffer, size, writer);
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
            BOOL ok = ::GetFileSizeEx (file_, &result);
            if (!ok) {
                DWORD const last_error = ::GetLastError ();
                std::ostringstream str;
                str << "Unable to get file size of " << pstore::quoted (path_);
                raise (::pstore::win32_erc (last_error), str.str ());
            }
            return result.QuadPart;
        }

        // truncate
        // ~~~~~~~~
        void file_handle::truncate (std::uint64_t size) {
            this->ensure_open ();

            auto const old_pos = this->tell ();
            this->seek (size);
            BOOL ok = ::SetEndOfFile (file_);
            auto const set_eof_error_code = ::GetLastError ();

            if (ok) {
                this->seek (std::min (size, old_pos));
            } else {
                std::ostringstream str;
                str << "Unable to set file size of " << pstore::quoted (path_);
                raise (::pstore::win32_erc (set_eof_error_code), str.str ());
            }
        }

        // lock
        // ~~~~
        bool file_handle::lock (std::uint64_t offset, std::size_t size, lock_kind kind,
                                blocking_mode block) {
            this->ensure_open ();

            OVERLAPPED overlapped;
            overlapped.Internal = 0;
            overlapped.InternalHigh = 0;
            overlapped.Offset = low4 (offset);
            overlapped.OffsetHigh = high4 (offset);
            overlapped.hEvent = static_cast<HANDLE> (0);

            DWORD flags = 0;
            switch (block) {
            case blocking_mode::non_blocking:
                flags |= LOCKFILE_FAIL_IMMEDIATELY;
                break;
            case blocking_mode::blocking:
                break;
            }
            switch (kind) {
            case lock_kind::exclusive_write:
                flags |= LOCKFILE_EXCLUSIVE_LOCK;
                break;
            case lock_kind::shared_read:
                break;
            }
            bool got_lock = true;
            BOOL ok = ::LockFileEx (file_, flags,
                                    0,            // "reserved, must be 0"
                                    low4 (size),  // number of bytes to lock (low)
                                    high4 (size), // number of bytes to lock (high)
                                    &overlapped); // "overlapped"
            if (!ok) {
                // If the LOCKFILE_FAIL_IMMEDIATELY flag is specified and an exclusive lock is
                // requested for a range of a file
                // that already has a shared or exclusive lock, the function returns the error
                // ERROR_IO_PENDING.
                DWORD const last_error = ::GetLastError ();
                if (block == blocking_mode::non_blocking && last_error == ERROR_IO_PENDING) {
                    got_lock = false;
                } else {
                    std::ostringstream str;
                    str << "Unable to lock range of " << pstore::quoted (path_);
                    raise (::pstore::win32_erc (last_error), str.str ());
                }
            }
            return got_lock;
        }

        // unlock
        // ~~~~~~
        void file_handle::unlock (std::uint64_t offset, std::size_t size) {
            this->ensure_open ();

            OVERLAPPED overlapped;
            overlapped.Internal = 0;
            overlapped.InternalHigh = 0;
            overlapped.Offset = low4 (offset);
            overlapped.OffsetHigh = high4 (offset);
            overlapped.hEvent = static_cast<HANDLE> (0);
            BOOL ok = ::UnlockFileEx (file_,
                                      0,            // "reserved, must be 0"
                                      low4 (size),  // number of bytes to unlock (low)
                                      high4 (size), // number of bytes to unlock (high)
                                      &overlapped); // "overlapped"
            if (!ok) {
                DWORD const last_error = ::GetLastError ();
                std::ostringstream str;
                str << "Unable to unlock range of " << pstore::quoted (path_);
                raise (win32_erc (last_error), str.str ());
            }
        }

        // latest_time
        // ~~~~~~~~~~~
        time_t file_handle::latest_time () const {
            file_handle local_file;
            HANDLE h = file_;
            if (!this->is_open ()) {
                local_file.open (path_, create_mode::open_existing, writable_mode::read_only,
                                 present_mode::must_exist);
                h = local_file.file_;
            }

            FILETIME creation_time;
            FILETIME last_access_time;
            FILETIME last_write_time;
            if (!::GetFileTime (h, &creation_time, &last_access_time, &last_write_time)) {
                DWORD const last_error = ::GetLastError ();
                std::ostringstream str;
                str << "Unable to get file time for " << pstore::quoted (path_);
                raise (win32_erc (last_error), str.str ());
            }

            auto compare = [](FILETIME const & lhs, FILETIME const & rhs) {
                return std::make_pair (lhs.dwHighDateTime, lhs.dwLowDateTime) <
                       std::make_pair (rhs.dwHighDateTime, rhs.dwLowDateTime);
            };
            auto const latest_time =
                std::max ({creation_time, last_access_time, last_write_time}, compare);
            return file_time_to_epoch (latest_time);
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
                raise (win32_erc (last_error), "GetTempPathW");
            } else if (num_code_units > temp_path.size ()) {
                // Increase the buffer size and try again.
                temp_path.resize (num_code_units);
                num_code_units =
                    ::GetTempPathW (static_cast<DWORD> (temp_path.size ()), temp_path.data ());
                if (num_code_units == 0) {
                    DWORD const last_error = ::GetLastError ();
                    raise (win32_erc (last_error), "GetTempPathW");
                } else if (num_code_units != temp_path.size ()) {
                    raise (std::errc::no_buffer_space, "GetTempPathW");
                }
            }

            return utf::win32::to8 (temp_path.data (), num_code_units);
        }



        namespace win32 {

            void deleter::platform_unlink (std::string const & path) {
                // Delete the file at the path given by the (UTF-8-encoded) parameter string.
                if (::DeleteFileW (pstore::utf::win32::to16 (path).c_str ()) == 0) {
                    DWORD const last_error = ::GetLastError ();
                    std::ostringstream str;
                    str << "Unable to delete file " << pstore::quoted (path);
                    raise (win32_erc (last_error), str.str ());
                }
            }

        } // namespace win32

        void unlink (std::string const & path) {
            // Delete the file at the path given by the (UTF-8-encoded) parameter string.
            if (::DeleteFileW (pstore::utf::win32::to16 (path).c_str ()) == 0) {
                DWORD const last_error = ::GetLastError ();
                std::ostringstream str;
                str << "Unable to delete file " << pstore::quoted (path);
                raise (win32_erc (last_error), str.str ());
            }
        }

        void rename (std::string const & from, std::string const & to) {
            std::wstring const fromw = pstore::utf::win32::to16 (from);
            std::wstring const tow = pstore::utf::win32::to16 (to);

            // Deliberately do not pass the MOVEFILE_COPY_ALLOWED to MoveFileExW() because this
            // could mean that the copy is anything but atomic. Do pass MOVEILE_REPLACE_EXISTING
            // to slightly more closely mirror the POSIX rename() behavior.
            BOOL ok = ::MoveFileExW (fromw.c_str (), tow.c_str (), MOVEFILE_REPLACE_EXISTING);
            if (!ok) {
                DWORD const last_error = ::GetLastError ();
                std::ostringstream message;
                message << "Unable to rename " << pstore::quoted (from) << " to "
                        << pstore::quoted (to);
                raise (win32_erc (last_error), message.str ());
            }
        }

    } // namespace file
} // namespace pstore
#endif // #ifdef _WIN32
// eof: lib/pstore_support/file_win32.cpp
