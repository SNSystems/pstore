//===- lib/romfs/directory.cpp --------------------------------------------===//
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
#include "pstore/romfs/directory.hpp"

#include <algorithm>
#include <cstring>

#include "pstore/romfs/dirent.hpp"

using namespace pstore;

namespace {

    // Returns true if the directory entry given by 'd' is a directory which references the
    // directory structure 'expected'.
    bool is_expected_dir (romfs::dirent const * const d,
                          gsl::not_null<romfs::directory const *> const expected) {
        if (d == nullptr) {
            return false;
        }
        auto const od = d->opendir ();
        return od && *od == expected;
    }

} // end anonymous namespace

namespace pstore {
    namespace romfs {

        // end
        // ~~~
        auto directory::end () const noexcept -> dirent const * { return begin () + size (); }

        // operator[]
        // ~~~~~~~~~~
        auto directory::operator[] (std::size_t const pos) const noexcept -> dirent const & {
            PSTORE_ASSERT (pos < size ());
            return members_[pos];
        }

        // find
        // ~~~~
        auto directory::find (gsl::not_null<directory const *> const d) const -> dirent const * {

            // This is a straightfoward linear search. Could be a performance problem in the future.
            auto const pos = std::find_if (begin (), end (), [d] (dirent const & de) {
                auto const od = de.opendir ();
                return od && od.get () == d;
            });
            return pos != end () ? &(*pos) : nullptr;
        }

        auto directory::find (gsl::not_null<char const *> const name, std::size_t length) const
            -> dirent const * {

            // Directories are sorted by name: we can use a binary search here.
            auto const end = this->end ();
            auto const it = std::lower_bound (
                this->begin (), end, std::make_pair (name, length),
                [] (dirent const & a,
                    std::pair<gsl::not_null<char const *>, std::size_t> const & b) {
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
        bool directory::check (gsl::not_null<directory const *> const parent,
                               check_stack_entry const * const visited) const {

            // This stops us from looping forever if we follow a directory pointer to a directory
            // that we have already visited.
            for (auto const * v = visited; v != nullptr; v = v->prev) {
                if (v->d == parent) {
                    return true;
                }
            }

            // Check that the directory entries are sorted by name.
            if (!std::is_sorted (begin (), end (), [] (dirent const & a, dirent const & b) {
                    return std::strcmp (a.name (), b.name ()) < 0;
                })) {
                return false;
            }

            // Look for the '.' and '..' entries and check that they point where we expect.
            if (!is_expected_dir (this->find ("."), this) ||
                !is_expected_dir (this->find (".."), parent)) {
                return false;
            }

            // Recursively check any directories contained by this one.
            for (dirent const & de : *this) {
                if (de.is_directory ()) {
                    if (auto const od = de.opendir ()) {
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

        bool directory::check () const { return this->check (this, nullptr); }

    } // end namespace romfs
} // end namespace pstore
