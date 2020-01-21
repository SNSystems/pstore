//*      _ _               _                    *
//*   __| (_)_ __ ___  ___| |_ ___  _ __ _   _  *
//*  / _` | | '__/ _ \/ __| __/ _ \| '__| | | | *
//* | (_| | | | |  __/ (__| || (_) | |  | |_| | *
//*  \__,_|_|_|  \___|\___|\__\___/|_|   \__, | *
//*                                      |___/  *
//===- lib/romfs/directory.cpp --------------------------------------------===//
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
#include "pstore/romfs/directory.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>

#include "pstore/romfs/dirent.hpp"
#include "pstore/support/gsl.hpp"

namespace {

    // Returns true if the directory entry given by 'd' is a directory which references the
    // directory structure 'expected'.
    bool is_expected_dir (pstore::romfs::dirent const * const PSTORE_NULLABLE d,
                          pstore::romfs::directory const * const PSTORE_NONNULL expected) {
        if (d == nullptr) {
            return false;
        }
        auto const od = d->opendir ();
        return od && *od == expected;
    }

} // end anonymous namespace


// end
// ~~~
auto pstore::romfs::directory::end () const -> dirent const * {
    return begin () + size ();
}

// operator[]
// ~~~~~~~~~~
auto pstore::romfs::directory::operator[] (std::size_t const pos) const noexcept -> dirent const & {
    assert (pos < size ());
    return members_[pos];
}

// find
// ~~~~
auto pstore::romfs::directory::find (directory const * const PSTORE_NONNULL d) const
    -> dirent const * PSTORE_NULLABLE {

    // This is a straightfoward linear search. Could be a performance problem in the future.
    auto const pos = std::find_if (begin (), end (), [d] (dirent const & de) {
        auto const od = de.opendir ();
        return od && od.get () == d;
    });
    return pos != end () ? &(*pos) : nullptr;
}

auto pstore::romfs::directory::find (char const * PSTORE_NONNULL name, std::size_t length) const
    -> dirent const * PSTORE_NULLABLE {

    // Directories are sorted by name: we can use a binary search here.
    auto const end = this->end ();
    auto const it = std::lower_bound (
        this->begin (), end, std::make_pair (name, length),
        [] (dirent const & a, std::pair<char const * PSTORE_NONNULL, std::size_t> const & b) {
            return std::strncmp (a.name (), b.first, b.second) < 0;
        });
    if (it != end) {
        gsl::czstring const n = it->name ();
        if (std::strncmp (n, name, length) == 0 && n[length] == '\0') {
            return it;
        }
    }
    return nullptr;
}

// check
// ~~~~~
bool pstore::romfs::directory::check (directory const * const PSTORE_NONNULL parent,
                                      check_stack_entry const * const PSTORE_NULLABLE
                                          visited) const {

    // This stops us from looping forever if we follow a directory pointer to a directory that we
    // have already visited.
    for (auto v = visited; v != nullptr; v = v->prev) {
        if (v->d == parent) {
            return true;
        }
    }

    // CHeck that the directory entries are sorted by name.
    if (!std::is_sorted (begin (), end (), [] (dirent const & a, dirent const & b) {
            return std::strcmp (a.name (), b.name ()) < 0;
        })) {
        return false;
    }

    // Look for the '.' and '..' entries and check that they point where we expect.
    if (!is_expected_dir (this->find ("."), this) || !is_expected_dir (this->find (".."), parent)) {
        return false;
    }

    // Recursively check any directories contained by this one.
    for (dirent const & de : *this) {
        if (de.is_directory ()) {
            if (error_or<directory const * PSTORE_NONNULL> const od = de.opendir ()) {
                check_stack_entry const me{*od, visited};
                if (!(*od)->check (this, &me)) {
                    return false;
                }
            } else {
                return false;
            }
        }
    }

    return true;
}

bool pstore::romfs::directory::check () const {
    return this->check (this, nullptr);
}
