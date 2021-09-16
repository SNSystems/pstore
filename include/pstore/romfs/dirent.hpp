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
            /// \param size_ File size in bytes.
            /// \param mtime_ Time when file data was last modified.
            /// \param mode_ File mode.
            constexpr stat (std::size_t const size_, mode_t const mode_,
                            std::time_t const mtime_) noexcept
                    : size{size_}
                    , mode{mode_}
                    , mtime{mtime_} {}

            constexpr bool operator== (stat const & rhs) const noexcept {
                return size == rhs.size && mtime == rhs.mtime && mode == rhs.mode;
            }
            constexpr bool operator!= (stat const & rhs) const noexcept {
                return !operator== (rhs);
            }

            std::size_t const size;  ///< File size in bytes.
            mode_t const mode;       ///< File mode.
            std::time_t const mtime; ///< Time when file data was last modified.
        };

        class directory;

        class dirent {
        public:
            constexpr dirent (gsl::not_null<gsl::czstring> const name,
                              gsl::not_null<void const *> const contents, stat const s) noexcept
                    : name_{name}
                    , contents_{contents}
                    , stat_{s} {}
            constexpr dirent (gsl::not_null<gsl::czstring> const name,
                              gsl::not_null<directory const *> const dir) noexcept
                    : name_{name}
                    , contents_{dir}
                    , stat_{sizeof (dir), mode_t::directory, 0 /*time*/} {}

            constexpr gsl::not_null<gsl::czstring> name () const noexcept { return name_; }
            constexpr gsl::not_null<void const *> contents () const noexcept { return contents_; }

            error_or<gsl::not_null<class directory const *>> opendir () const;

            constexpr struct stat const & stat () const noexcept { return stat_; }
            constexpr bool is_directory () const noexcept {
                return stat_.mode == mode_t::directory;
            }

        private:
            gsl::not_null<gsl::czstring> const name_;
            gsl::not_null<void const *> const contents_;
            struct stat stat_;
        };

    } // end namespace romfs
} // end namespace pstore

#endif // PSTORE_ROMFS_DIRENT_HPP
