//===- include/pstore/romfs/dirent.hpp --------------------*- mode: C++ -*-===//
//*      _ _                _    *
//*   __| (_)_ __ ___ _ __ | |_  *
//*  / _` | | '__/ _ \ '_ \| __| *
//* | (_| | | | |  __/ | | | |_  *
//*  \__,_|_|_|  \___|_| |_|\__| *
//*                              *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_ROMFS_DIRENT_HPP
#define PSTORE_ROMFS_DIRENT_HPP

#include <cstdint>
#include <ctime>

#include <sys/types.h>

#include "pstore/adt/error_or.hpp"
#include "pstore/support/gsl.hpp"

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
