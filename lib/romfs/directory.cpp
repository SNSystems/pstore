//*      _ _               _                    *
//*   __| (_)_ __ ___  ___| |_ ___  _ __ _   _  *
//*  / _` | | '__/ _ \/ __| __/ _ \| '__| | | | *
//* | (_| | | | |  __/ (__| || (_) | |  | |_| | *
//*  \__,_|_|_|  \___|\___|\__\___/|_|   \__, | *
//*                                      |___/  *
//===- lib/romfs/directory.cpp --------------------------------------------===//
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
#include "pstore/romfs/directory.hpp"
#include <algorithm>
#include <cstring>
#include <string>
#include "pstore/romfs/dirent.hpp"

auto pstore::romfs::directory::end () const -> dirent const * {
    return begin () + size ();
}

auto pstore::romfs::directory::operator[] (std::size_t pos) const noexcept -> dirent const & {
    assert (pos < size ());
    return members_[pos];
}

auto pstore::romfs::directory::find (directory const * const PSTORE_NONNULL d) const
    -> dirent const * PSTORE_NULLABLE {

    // TODO: this is a straightfoward linear search. Could this be a performance problem?
    auto pos = std::find_if (begin (), end (), [d](dirent const & de) {
        auto const od = de.opendir ();
        return od.has_value () && od.get_value () == d;
    });
    return pos != end () ? &(*pos) : nullptr;
}

auto pstore::romfs::directory::find (char const * PSTORE_NONNULL name, std::size_t length) const
    -> dirent const * PSTORE_NULLABLE {

    auto end = this->end ();
    auto it = std::lower_bound (
        this->begin (), end, std::make_pair (name, length),
        [](dirent const & a, std::pair<char const * PSTORE_NONNULL, std::size_t> const & b) {
            return std::strncmp (a.name (), b.first, b.second) < 0;
        });
    if (it != end) {
        gsl::czstring n = it->name ();
        if (std::strncmp (n, name, length) == 0 && n[length] == '\0') {
            return it;
        }
    }
    return nullptr;
}

bool pstore::romfs::directory::check (directory const * const PSTORE_NONNULL parent) const {
    auto comp = [](dirent const & a, dirent const & b) {
        return std::strcmp (a.name (), b.name ()) < 0;
    };
    if (!std::is_sorted (begin (), end (), comp)) {
        return false;
    }

    auto isdir = [](dirent const * const PSTORE_NULLABLE d,
                    directory const * const PSTORE_NONNULL expected) {
        if (d == nullptr) {
            return false;
        }
        error_or<directory const * PSTORE_NONNULL> od = d->opendir ();
        return od.has_value () && od.get_value () == expected;
    };
    if (!isdir (this->find (".", 1), this) && isdir (this->find ("..", 2), parent)) {
        return false;
    }

    for (dirent const & de : *this) {
        char const * name = de.name ();
        if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
            continue;
        }
        if (de.is_directory ()) {
            error_or<directory const * PSTORE_NONNULL> od = de.opendir ();
            if (od.has_error ()) {
                return false;
            }
            if (!od.get_value ()->check (this)) {
                return false;
            }
        }
    }

    return true;
}
