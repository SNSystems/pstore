//*      _ _               _                    *
//*   __| (_)_ __ ___  ___| |_ ___  _ __ _   _  *
//*  / _` | | '__/ _ \/ __| __/ _ \| '__| | | | *
//* | (_| | | | |  __/ (__| || (_) | |  | |_| | *
//*  \__,_|_|_|  \___|\___|\__\___/|_|   \__, | *
//*                                      |___/  *
//===- include/pstore/romfs/directory.hpp ---------------------------------===//
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
#ifndef PSTORE_ROMFS_DIRECTORY_HPP
#define PSTORE_ROMFS_DIRECTORY_HPP

#include <cassert>
#include <cstdlib>

#include "pstore/support/gsl.hpp"
#include "pstore/support/portab.hpp"

namespace pstore {
    namespace romfs {

        class dirent;

        class directory {
        public:
            constexpr directory (std::size_t size, dirent const * PSTORE_NONNULL members) noexcept
                    : size_{size}
                    , members_{members} {}
            template <std::size_t N>
            constexpr directory (dirent const (&members)[N]) noexcept
                    : directory (N, members) {}

            template <typename Container>
            constexpr directory (Container const & c) noexcept
                    : directory (c.size (), c.data ()) {}

            directory (directory const &) = delete;
            directory & operator= (directory const &) = delete;

            dirent const * PSTORE_NONNULL begin () const { return members_; }
            dirent const * PSTORE_NONNULL end () const;
            std::size_t size () const noexcept { return size_; }
            dirent const & operator[] (std::size_t pos) const noexcept;

            // note that name is not necessarily nul terminated!
            dirent const * PSTORE_NULLABLE find (char const * PSTORE_NONNULL name,
                                                 std::size_t length) const;

            template <std::size_t Size>
            dirent const * PSTORE_NULLABLE find (char const (&name)[Size]) const {
                return this->find (name, Size - 1U);
            }

            dirent const * PSTORE_NULLABLE find (directory const * PSTORE_NONNULL d) const;

            bool check (directory const * const PSTORE_NONNULL parent) const;

        private:
            std::size_t const size_;
            dirent const * PSTORE_NONNULL members_;
        };

    } // end namespace romfs
} // end namespace pstore

#endif // PSTORE_ROMFS_DIRECTORY_HPP
