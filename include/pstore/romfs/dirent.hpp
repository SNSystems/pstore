//*      _ _                _    *
//*   __| (_)_ __ ___ _ __ | |_  *
//*  / _` | | '__/ _ \ '_ \| __| *
//* | (_| | | | |  __/ | | | |_  *
//*  \__,_|_|_|  \___|_| |_|\__| *
//*                              *
//===- include/pstore/romfs/dirent.hpp ------------------------------------===//
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
#ifndef PSTORE_ROMFS_DIRENT_HPP
#define PSTORE_ROMFS_DIRENT_HPP

#include <cassert>
#include <cstdint>
#include <ctime>

#include <sys/types.h>

#include "pstore/support/error_or.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/portab.hpp"

namespace pstore {
    namespace romfs {

        enum class mode_t { file, directory };

        struct stat {
            constexpr stat (std::size_t const size_, std::time_t const mtime_,
                            mode_t const m) noexcept
                    : size{size_}
                    , mtime{mtime_}
                    , mode{m} {}

            constexpr bool operator== (stat const & rhs) const noexcept {
                return size == rhs.size && mtime == rhs.mtime && mode == rhs.mode;
            }
            constexpr bool operator!= (stat const & rhs) const noexcept {
                return !operator== (rhs);
            }

            std::size_t size;  ///< File size in bytes.
            std::time_t mtime; ///< Time when file data was last modified.
            mode_t mode;
        };

        class directory;

        class dirent {
        public:
            constexpr dirent (gsl::czstring const PSTORE_NONNULL name,
                              void const * const PSTORE_NONNULL contents, stat const s) noexcept
                    : name_{name}
                    , contents_{contents}
                    , stat_{s} {}
            constexpr dirent (gsl::czstring const PSTORE_NONNULL name,
                              directory const * const PSTORE_NONNULL dir) noexcept
                    : name_{name}
                    , contents_{dir}
                    , stat_{sizeof (dir), 0 /*time*/, mode_t::directory} {}

            constexpr gsl::czstring PSTORE_NONNULL name () const noexcept { return name_; }
            constexpr void const * PSTORE_NONNULL contents () const noexcept { return contents_; }

            error_or<class directory const * PSTORE_NONNULL> opendir () const;

            constexpr struct stat const & stat () const noexcept { return stat_; }
            constexpr bool is_directory () const noexcept {
                return stat_.mode == mode_t::directory;
            }

        private:
            gsl::czstring PSTORE_NONNULL name_;
            void const * PSTORE_NONNULL contents_;
            struct stat stat_;
        };

    } // end namespace romfs
} // end namespace pstore

#endif // PSTORE_ROMFS_DIRENT_HPP
