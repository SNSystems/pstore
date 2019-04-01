//*                       __      *
//*  _ __ ___  _ __ ___  / _|___  *
//* | '__/ _ \| '_ ` _ \| |_/ __| *
//* | | | (_) | | | | | |  _\__ \ *
//* |_|  \___/|_| |_| |_|_| |___/ *
//*                               *
//===- include/pstore/romfs/romfs.hpp -------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_ROMFS_ROMFS_HPP
#define PSTORE_ROMFS_ROMFS_HPP

#include <cerrno>
#include <cstdlib>
#include <memory>
#include <string>
#include <system_error>
#include <tuple>

#ifndef _WIN32
#    include <sys/types.h>
#endif

#include "pstore/romfs/directory.hpp"
#include "pstore/romfs/dirent.hpp"
#include "pstore/support/error_or.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace romfs {

        enum class error_code {
            einval = EINVAL,
            enoent = ENOENT,
            enotdir = ENOTDIR,
        };

        class error_category : public std::error_category {
        public:
            // The need for this constructor was removed by CWG defect 253 but Clang (prior
            // to 3.9.0) and GCC (before 4.6.4) require its presence.
            error_category () noexcept {} // NOLINT
            char const * PSTORE_NONNULL name () const noexcept override;
            std::string message (int error) const override;
        };

    } // end namespace romfs
} // end namespace pstore

namespace std {

    template <>
    struct is_error_code_enum<pstore::romfs::error_code> : std::true_type {};

    std::error_code make_error_code (pstore::romfs::error_code e);

} // namespace std

namespace pstore {
    namespace romfs {
        class open_file;
        class open_directory;

        //*     _                _      _            *
        //*  __| |___ ___ __ _ _(_)_ __| |_ ___ _ _  *
        //* / _` / -_|_-</ _| '_| | '_ \  _/ _ \ '_| *
        //* \__,_\___/__/\__|_| |_| .__/\__\___/_|   *
        //*                       |_|                *
        class descriptor {
            friend class romfs;

        public:
            descriptor (descriptor const & other) = default;
            descriptor (descriptor && other) = default;
            ~descriptor () noexcept = default;

            descriptor & operator= (descriptor const & other) = default;
            descriptor & operator= (descriptor && other) = default;

            std::size_t read (void * PSTORE_NONNULL buffer, std::size_t size, std::size_t count);
            error_or<std::size_t> seek (off_t offset, int whence);
            struct stat stat () const;

        private:
            explicit descriptor (std::shared_ptr<open_file> f)
                    : f_{std::move (f)} {}
            // Using a shared_ptr<> here so that descriptor instances can be passed around in
            // the same way as they would if 'descriptor' was the int type that's traditionally
            // used to represent file descriptors.
            std::shared_ptr<open_file> f_;
        };

        //*     _ _             _        _                _      _            *
        //*  __| (_)_ _ ___ _ _| |_   __| |___ ___ __ _ _(_)_ __| |_ ___ _ _  *
        //* / _` | | '_/ -_) ' \  _| / _` / -_|_-</ _| '_| | '_ \  _/ _ \ '_| *
        //* \__,_|_|_| \___|_||_\__| \__,_\___/__/\__|_| |_| .__/\__\___/_|   *
        //*                                                |_|                *
        class dirent_descriptor {
            friend class romfs;

        public:
            dirent const * PSTORE_NULLABLE read ();
            void rewind ();

        private:
            explicit dirent_descriptor (std::shared_ptr<open_directory> f)
                    : f_{std::move (f)} {}
            std::shared_ptr<open_directory> f_;
        };


        //*                 __     *
        //*  _ _ ___ _ __  / _|___ *
        //* | '_/ _ \ '  \|  _(_-< *
        //* |_| \___/_|_|_|_| /__/ *
        //*                        *
        class romfs {
        public:
            explicit romfs (directory const * PSTORE_NONNULL root);
            romfs (romfs const &) noexcept = default;
            romfs (romfs &&) noexcept = default;
            ~romfs () noexcept = default;

            romfs & operator= (romfs const &) = delete;
            romfs & operator= (romfs &&) = delete;

            error_or<descriptor> open (gsl::czstring PSTORE_NONNULL path) const;
            error_or<dirent_descriptor> opendir (gsl::czstring PSTORE_NONNULL path);
            error_or<struct stat> stat (gsl::czstring PSTORE_NONNULL path) const;

            error_or<std::string> getcwd () const;
            std::error_code chdir (gsl::czstring PSTORE_NONNULL path);

            /// \brief Check that the file system's structures are intact.
            ///
            /// Since the data is read-only there should be no need to call this function except as a belf-and-braces debug check.
            bool fsck () const;

        private:
            using dirent_ptr = dirent const * PSTORE_NONNULL;

            error_or<std::string> dir_to_string (directory const * const PSTORE_NONNULL dir) const;

            static dirent const * PSTORE_NONNULL
            directory_to_dirent (directory const * PSTORE_NONNULL d);

            static directory const * PSTORE_NONNULL
            dirent_to_directory (dirent const * PSTORE_NONNULL de);

            error_or<dirent_ptr> parse_path (gsl::czstring PSTORE_NONNULL path) const {
                return parse_path (path, cwd_);
            }

            /// Parse a path string returning the directory-entry to which it refers or an error.
            /// Paths follow the POSIX convention of using a slash ('/') to separate components. A
            /// leading slash indicates that the search should start at the file system's root
            /// directory rather than the default directory given by the \p start_dir argument.
            ///
            /// \param path The path string to be parsed.
            /// \param start_dir  The directory to which the path is relative. Ignored if the
            /// initial character of the \p path argument is a slash.
            /// \returns  The directory entry described by the \p path argument or an error if the
            /// string was not valid.
            error_or<dirent_ptr> parse_path (gsl::czstring PSTORE_NONNULL path,
                                             directory const * PSTORE_NONNULL start_dir) const;


            directory const * const PSTORE_NONNULL root_;
            directory const * PSTORE_NONNULL cwd_;
        };

    } // end namespace romfs
} // end namespace pstore

#endif // PSTORE_ROMFS_ROMFS_HPP
