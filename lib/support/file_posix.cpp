//*   __ _ _       *
//*  / _(_) | ___  *
//* | |_| | |/ _ \ *
//* |  _| | |  __/ *
//* |_| |_|_|\___| *
//*                *
//===- lib/support/file_posix.cpp -----------------------------------------===//
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
/// \file file_posix.cpp
/// \brief POSIX implementation of the cross-platform file APIs

#ifndef _WIN32

#include "pstore/support/file.hpp"

// standard includes
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <sstream>

// platform includes
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "pstore/support/small_vector.hpp"
#include "pstore/support/path.hpp"
#include "pstore/support/quoted_string.hpp"


namespace {

    template <typename MessageStr, typename PathStr>
    PSTORE_NO_RETURN void raise_file_error (int err, MessageStr message, PathStr path) {
        std::ostringstream str;
        str << message << " \"" << path << "\"";
        raise (::pstore::errno_erc{err}, str.str ());
    }

} // end anonymous namespace


namespace pstore {
    namespace file {

        //*   __ _ _       _                 _ _      *
        //*  / _(_) |___  | |_  __ _ _ _  __| | |___  *
        //* |  _| | / -_) | ' \/ _` | ' \/ _` | / -_) *
        //* |_| |_|_\___| |_||_\__,_|_||_\__,_|_\___| *
        //*                                           *
        // open
        // ~~~~
        void file_handle::open (std::string const & path, create_mode create,
                                writable_mode writable, present_mode present) {
            this->close ();

            path_ = path;
            is_writable_ = writable == writable_mode::read_write;

            int oflag = is_writable_ ? O_RDWR : O_RDONLY;
            switch (create) {
            // Creates a new file, only if it does not already exist
            case create_mode::create_new: oflag |= O_CREAT | O_EXCL; break;
            // Opens a file only if it already exists
            case create_mode::open_existing: break;
            // Opens an existing file if present, and creates a new file otherwise.
            case create_mode::open_always: oflag |= O_CREAT; break;
            }

            // user, group, and others have read permission.
            mode_t pmode = S_IRUSR | S_IRGRP | S_IROTH;
            if (is_writable_) {
                // user, group, and others have read and write permission.
                pmode |= S_IWUSR | S_IWGRP | S_IWOTH;
            }

            file_ = ::open (path.c_str (), oflag, pmode); // NOLINT
            if (file_ == -1) {
                int const err = errno;
                if (present == present_mode::allow_not_found && err == ENOENT) {
                    file_ = -1;
                } else {
                    raise_file_error (err, "Unable to open", path);
                }
            }
        }

        void file_handle::open (unique, std::string const & directory) {
            this->close ();

            // assert (path::is_directory (directory));
            // TODO: path::join should be able to write to a back_inserter so that we can avoid this
            // intermediate string object.
            std::string const path = path::join (directory, "pst-XXXXXX");

            // mkstemp() modifies its input parameter so that on return it contains
            // the actual name of the temporary file that was created.
            small_vector<char> buffer (path.length () + 1);
            auto out = std::copy (std::begin (path), std::end (path), std::begin (buffer));
            *out = '\0';

            file_ = ::mkstemp (buffer.data ());
            int const err = errno;
            path_ = buffer.data ();
            is_writable_ = true;

            if (file_ == -1) {
                raise (::pstore::errno_erc{err}, "Unable to create temporary file");
            }
        }

        void file_handle::open (temporary, std::string const & directory) {
            this->open (unique{}, directory);
            if (::unlink (this->path ().c_str ()) == -1) {
                int const err = errno;
                raise (::pstore::errno_erc{err}, "Unable to delete temporary file");
            }
        }

        // close
        // ~~~~~
        void file_handle::close () {
            if (file_ != invalid_oshandle) {
                bool ok = ::close (file_) != -1;
                file_ = invalid_oshandle;
                if (!ok) {
                    int const err = errno;
                    raise_file_error (err, "Unable to close", this->path ());
                }
                is_writable_ = false;
            }
        }

        // seek
        // ~~~~
        void file_handle::seek (std::uint64_t position) {
            static_assert (std::numeric_limits<off_t>::max () >=
                               std::numeric_limits<std::int64_t>::max (),
                           "off_t cannot represent all file positions");
            this->ensure_open ();

            // FIXME: if position > std::numeric_limits <off_t>?
            off_t r = ::lseek (file_, static_cast<off_t> (position), SEEK_SET);
            if (r == off_t{-1}) {
                int const err = errno;
                raise_file_error (err, "lseek/SEEK_SET failed", this->path ());
            }
        }

        // tell
        // ~~~~
        std::uint64_t file_handle::tell () {
            this->ensure_open ();

            off_t r = ::lseek (file_, off_t{0}, SEEK_CUR);
            if (r == off_t{-1}) {
                int const err = errno;
                raise_file_error (err, "lseek/SEEK_CUR failed", this->path ());
            }
            assert (r >= 0);
            return static_cast<std::uint64_t> (r);
        }

        // read_buffer
        // ~~~~~~~~~~~
        std::size_t file_handle::read_buffer (::pstore::gsl::not_null<void *> buffer,
                                              std::size_t nbytes) {
            // TODO: handle the case where nbytes > ssize_t max.
            assert (nbytes <= std::numeric_limits<ssize_t>::max ());
            this->ensure_open ();

            ssize_t const r = ::read (file_, buffer.get (), nbytes);
            if (r < 0) {
                int const err = (r == -1) ? errno : EINVAL;
                raise_file_error (err, "read failed", this->path ());
            }
            return static_cast<std::size_t> (r);
        }

        // write_buffer
        // ~~~~~~~~~~~~
        void file_handle::write_buffer (::pstore::gsl::not_null<void const *> buffer,
                                        std::size_t nbytes) {
            this->ensure_open ();

            ssize_t r = ::write (file_, buffer.get (), nbytes);
            if (r == -1) {
                int const err = errno;
                raise_file_error (err, "write failed", this->path ());
            }

            // If the write call succeeded, then the file must have been writable!
            assert (is_writable_);
        }

        // size
        // ~~~~
        // FIXME: does this function need to be aware of symbolic links?
        std::uint64_t file_handle::size () {
            this->ensure_open ();
            struct stat buf {};
            int erc = fstat (file_, &buf);
            if (erc == -1) {
                int const err = errno;
                raise_file_error (err, "fstat failed", this->path ());
            }

            static_assert (std::numeric_limits<std::uint64_t>::max () >=
                               std::numeric_limits<decltype (buf.st_size)>::max (),
                           "stat.st_size is too large for uint64_t");
            assert (buf.st_size >= 0);
            return static_cast<std::uint64_t> (buf.st_size);
        }

        // truncate
        // ~~~~~~~~
        void file_handle::truncate (std::uint64_t size) {
            this->ensure_open ();
            // TODO: should this function (and other, similar, functions) throw here rather than
            // assert?
            assert (size < std::numeric_limits<off_t>::max ());
            int erc = ftruncate (file_, static_cast<off_t> (size));
            if (erc == -1) {
                int const err = errno;
                raise_file_error (err, "ftruncate failed", this->path ());
            }
        }

        // lock_reg
        // ~~~~~~~~
        /// A helper function for the lock() and unlock() methods. It is a simple wrapper for the
        /// fcntl() system call which fills in all of the fields of the flock struct as necessary.
        //[static]
        int file_handle::lock_reg (int fd, int cmd, short type, off_t offset, short whence,
                                   off_t len) {
            struct flock lock {};
            lock.l_type = type;     // type of lock: F_RDLCK, F_WRLCK, F_UNLCK
            lock.l_whence = whence; // how to interpret l_start (SEEK_SET/SEEK_CUR/SEEK_END)
            lock.l_start = offset;  // starting offset for lock
            lock.l_len = len;       // number of bytes to lock
            lock.l_pid = 0;         // PID of blocking process (set by F_GETLK and F_OFD_GETLK)
            return fcntl (fd, cmd, &lock); // NOLINT
        }

        // lock
        // ~~~~
        bool file_handle::lock (std::uint64_t offset, std::size_t size, lock_kind kind,
                                blocking_mode block) {
            this->ensure_open ();

            assert (offset < std::numeric_limits<off_t>::max ());
            assert (size < std::numeric_limits<off_t>::max ());

            int cmd = 0;
            switch (block) {
            case blocking_mode::non_blocking: cmd = F_SETLK; break;
            case blocking_mode::blocking: cmd = F_SETLKW; break;
            }

            short type = 0;
            switch (kind) {
            case lock_kind::shared_read: type = F_RDLCK; break;
            case lock_kind::exclusive_write: type = F_WRLCK; break;
            };

            bool got_lock = true;
            if (file_handle::lock_reg (file_,
                                       cmd, // set a file lock (maybe a blocking one),
                                       type, static_cast<off_t> (offset), SEEK_SET,
                                       static_cast<off_t> (size)) != 0) {
                int const err = errno;
                if (block == blocking_mode::non_blocking && (err == EACCES || err == EAGAIN)) {
                    // The cmd argument is F_SETLK; the type of lock (l_type) is a shared (F_RDLCK)
                    // or exclusive (F_WRLCK) lock and the segment of a file to be locked is already
                    // exclusive-locked by another process, or the type is an exclusive lock and
                    // some portion of the segment of a file to be locked is already shared-locked
                    // or exclusive-locked by another process
                    got_lock = false;
                } else {
                    raise_file_error (err, "fcntl/lock failed", this->path ());
                }
            }
            return got_lock;
        }

        // unlock
        // ~~~~~~
        void file_handle::unlock (std::uint64_t offset, std::size_t size) {
            this->ensure_open ();

            assert (offset < std::numeric_limits<off_t>::max ());
            assert (size < std::numeric_limits<off_t>::max ());

            if (file_handle::lock_reg (file_, F_SETLK,
                                       F_UNLCK, // release an existing lock
                                       static_cast<off_t> (offset), SEEK_SET,
                                       static_cast<off_t> (size)) != 0) {
                int const err = errno;
                raise_file_error (err, "fcntl/unlock failed", this->path ());
            }
        }

        // latest_time
        // ~~~~~~~~~~~
        std::time_t file_handle::latest_time () const {
            struct stat buf {};
            ::stat (path_.c_str (), &buf);
/// The time members of struct stat might be called st_Xtimespec (of type struct timespec) or
/// st_Xtime (and be of type time_t). This macro is defined in the former situation.
#ifdef PSTORE_STAT_TIMESPEC
            auto compare = [](struct timespec const & lhs, struct timespec const & rhs) {
                return std::make_pair (lhs.tv_sec, lhs.tv_nsec) <
                       std::make_pair (rhs.tv_sec, rhs.tv_nsec);
            };
            auto const t =
                std::max ({buf.st_atimespec, buf.st_mtimespec, buf.st_ctimespec}, compare);
            constexpr auto nano = 1000000000;
            return t.tv_sec + (t.tv_nsec + (nano / 2)) / nano;
#else
            return std::max ({buf.st_atime, buf.st_mtime, buf.st_ctime});
#endif
        }

        // get_temporary_directory
        // ~~~~~~~~~~~~~~~~~~~~~~~
        // [static]
        std::string file_handle::get_temporary_directory () {
            // Following boost filesystem, we check some select environment variables
            // for user temporary directories before resorting to /tmp.
            static constexpr std::array<char const *, 4> env_var_names{{
                "TMPDIR",
                "TMP",
                "TEMP",
                "TEMPDIR",
            }};
            for (auto name : env_var_names) {
                if (char const * val = std::getenv (name)) {
                    return val;
                }
            }

            return "/tmp";
        }


        namespace posix {

            void deleter::platform_unlink (std::string const & path) {
                if (::unlink (path.c_str ()) == -1) {
                    int const err = errno;
                    raise_file_error (err, "unlink failed", path);
                }
            }

        } // namespace posix


        bool exists (std::string const & path) { return ::access (path.c_str (), F_OK) != -1; }

        void rename (std::string const & from, std::string const & to) {
            int erc = ::rename (from.c_str (), to.c_str ());
            if (erc == -1) {
                int const last_error = errno;

                std::ostringstream message;
                message << "Unable to rename " << quoted (from) << " to " << quoted (to);
                raise (errno_erc{last_error}, message.str ());
            }
        }

        void unlink (std::string const & path) {
            if (::unlink (path.c_str ()) == -1) {
                int const err = errno;
                raise_file_error (err, "unlink failed", path);
            }
        }
    } // namespace file
} // namespace pstore
#endif //_WIN32
// eof: lib/support/file_posix.cpp
