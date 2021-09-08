//===- include/pstore/romfs/directory.hpp -----------------*- mode: C++ -*-===//
//*      _ _               _                    *
//*   __| (_)_ __ ___  ___| |_ ___  _ __ _   _  *
//*  / _` | | '__/ _ \/ __| __/ _ \| '__| | | | *
//* | (_| | | | |  __/ (__| || (_) | |  | |_| | *
//*  \__,_|_|_|  \___|\___|\__\___/|_|   \__, | *
//*                                      |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_ROMFS_DIRECTORY_HPP
#define PSTORE_ROMFS_DIRECTORY_HPP

#include <cstdlib>

#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace romfs {

        class dirent;

        class directory {
        public:
            constexpr directory (std::size_t const size,
                                 gsl::not_null<dirent const *> const members) noexcept
                    : size_{size}
                    , members_{members} {}
            template <std::size_t N>
            explicit constexpr directory (dirent const (&members)[N]) noexcept
                    : directory (N, members) {}

            template <typename Container>
            explicit constexpr directory (Container const & c) noexcept
                    : directory (c.size (), c.data ()) {}

            directory (directory const &) = delete;
            directory (directory &&) = delete;

            ~directory () noexcept = default;

            directory & operator= (directory const &) = delete;
            directory & operator= (directory &&) = delete;

            using iterator = dirent const *;
            iterator begin () const noexcept { return members_; }
            iterator end () const noexcept;

            std::size_t size () const noexcept { return size_; }
            dirent const & operator[] (std::size_t pos) const noexcept;

            /// Search the directory for a member whose name equals the string described by \p name
            /// and \p length.
            ///
            /// \note \p name is not a nul terminated string.
            ///
            /// \param name  The name of the entry to be found.
            /// \param length  The number of code units in \p name.
            /// \returns A pointer to the relevant directory entry, if the name was found or nullptr
            ///          if it was not.
            dirent const * find (gsl::not_null<char const *> name, std::size_t length) const;

            template <std::size_t Size>
            dirent const * find (char const (&name)[Size]) const {
                return this->find (&name[0], Size - 1U);
            }

            /// Searchs the directory for a member which references the directory structure \p d.
            ///
            /// \param d  The directory to be found.
            /// \returns  A pointer to the directory entry if found, or nullptr if not.
            dirent const * find (gsl::not_null<directory const *> d) const;

            /// Performs basic validity checks on a directory hierarchy.
            bool check () const;

        private:
            struct check_stack_entry {
                gsl::not_null<directory const *> d;
                check_stack_entry const * prev;
            };
            bool check (gsl::not_null<directory const *> const parent,
                        check_stack_entry const * const visited) const;

            /// The number of entries in the members_ array.
            std::size_t const size_;
            /// An array of directory members.
            gsl::not_null<dirent const *> const members_;
        };

    } // end namespace romfs
} // end namespace pstore

#endif // PSTORE_ROMFS_DIRECTORY_HPP
