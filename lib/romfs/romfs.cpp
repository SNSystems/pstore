//*                       __      *
//*  _ __ ___  _ __ ___  / _|___  *
//* | '__/ _ \| '_ ` _ \| |_/ __| *
//* | | | (_) | | | | | |  _\__ \ *
//* |_|  \___/|_| |_| |_|_| |___/ *
//*                               *
//===- lib/romfs/romfs.cpp ------------------------------------------------===//
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
#include "pstore/romfs/romfs.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

#include "pstore/romfs/directory.hpp"
#include "pstore/romfs/dirent.hpp"

namespace pstore {
    namespace romfs {

        char const * error_category::name () const noexcept { return "pstore-romfs category"; }

        std::string error_category::message (int error) const {
            switch (static_cast<error_code> (error)) {
            case error_code::einval: return "EINVAL";
            case error_code::enoent: return "ENOENT";
            case error_code::enotdir: return "ENOTDIR";
            }
            return "unknown error";
        }

        std::error_category const & get_error_category () {
            static error_category const cat;
            return cat;
        }

    } // end namespace romfs
} // end namespace pstore

namespace std {

    std::error_code make_error_code (pstore::romfs::error_code e) {
        static_assert (std::is_same<std::underlying_type<decltype (e)>::type, int>::value,
                       "base type of error_code must be int to permit safe static cast");
        return {static_cast<int> (e), pstore::romfs::get_error_category ()};
    }

} // namespace std

namespace {

    std::pair<char const *, pstore::gsl::czstring> path_component (pstore::gsl::czstring path) {
        if (path == nullptr) {
            return {nullptr, nullptr};
        }
        char const * p = path;
        for (; *p != '\0' && *p != '/'; ++p) {
        }
        return {path, p};
    }

    pstore::gsl::czstring next_component (pstore::gsl::czstring p) {
        if (p == nullptr) {
            return p;
        }
        for (; *p == '/'; ++p) {
        }
        return p;
    }

} // end anonymous namespace

namespace pstore {
    namespace romfs {

        //*                        __ _ _      *
        //*  ___ _ __  ___ _ _    / _(_) |___  *
        //* / _ \ '_ \/ -_) ' \  |  _| | / -_) *
        //* \___/ .__/\___|_||_| |_| |_|_\___| *
        //*     |_|                            *
        class open_file {
        public:
            explicit open_file (dirent const & d)
                    : dir_{d} {}
            open_file (open_file const &) = delete;
            open_file & operator= (open_file const &) = delete;

            error_or<off_t> seek (off_t offset, int whence);
            std::size_t read (void * PSTORE_NONNULL buffer, std::size_t size, std::size_t count);
            struct stat stat () {
                return dir_.stat ();
            }

        private:
            dirent const & dir_;
            off_t pos_ = 0;
        };

        // read
        // ~~~~
        std::size_t open_file::read (void * PSTORE_NONNULL buffer, std::size_t size,
                                     std::size_t count) {
            if (size == 0 || count == 0) {
                return 0;
            }
            auto const file_size =
                std::make_unsigned<off_t>::type (std::max (dir_.stat ().st_size, off_t{0}));
            auto num_read = std::size_t{0};
            auto start = reinterpret_cast<std::uint8_t const *> (dir_.contents ()) + pos_;
            for (; num_read < count; ++num_read) {
                if (pos_ + size > file_size) {
                    return num_read;
                }
                std::memcpy (buffer, start, size);
                start += size;
                buffer = reinterpret_cast<std::uint8_t *> (buffer) + size;
                pos_ += size;
            }
            return num_read;
        }

        // seek
        // ~~~~
        error_or<off_t> open_file::seek (off_t offset, int whence) {
            off_t new_pos;
            switch (whence) {
            case SEEK_SET: new_pos = offset; break;
            case SEEK_CUR: new_pos = pos_ + offset; break;
            case SEEK_END: new_pos = dir_.stat ().st_size + offset; break;
            default:
                // Whence is not a proper value.
                return error_or<off_t> (std::make_error_code (error_code::einval));
            }

            if (new_pos < 0) {
                // The seek location (calculated from offset and whence) is negative.
                return error_or<off_t> (std::make_error_code (error_code::einval));
            }

            // FIXME: limit to the file size!
            pos_ = new_pos;
            return error_or<off_t>{new_pos};
        }


        //*                          _ _            _                 *
        //*  ___ _ __  ___ _ _    __| (_)_ _ ___ __| |_ ___ _ _ _  _  *
        //* / _ \ '_ \/ -_) ' \  / _` | | '_/ -_) _|  _/ _ \ '_| || | *
        //* \___/ .__/\___|_||_| \__,_|_|_| \___\__|\__\___/_|  \_, | *
        //*     |_|                                             |__/  *
        class open_directory {
        public:
            explicit open_directory (directory const & dir) noexcept
                    : dir_{dir} {}
            open_directory (open_directory const &) = delete;
            open_directory & operator= (open_file const &) = delete;

            void rewind () noexcept { index_ = 0U; }
            dirent const * PSTORE_NULLABLE read ();

        private:
            directory const & dir_;
            unsigned index_ = 0;
        };

        auto open_directory::read () -> dirent const * PSTORE_NULLABLE {
            if (index_ >= dir_.size ()) {
                return nullptr;
            }
            return &(dir_[index_++]);
        }


        //*     _                _      _            *
        //*  __| |___ ___ __ _ _(_)_ __| |_ ___ _ _  *
        //* / _` / -_|_-</ _| '_| | '_ \  _/ _ \ '_| *
        //* \__,_\___/__/\__|_| |_| .__/\__\___/_|   *
        //*                       |_|                *
        std::size_t descriptor::read (void * PSTORE_NONNULL buffer, std::size_t size,
                                      std::size_t count) {
            return f_->read (buffer, size, count);
        }
        error_or<off_t> descriptor::seek (off_t offset, int whence) {
            return f_->seek (offset, whence);
        }
        auto descriptor::stat () const -> struct stat {
            return f_->stat ();
        }


        //*     _ _             _        _                _      _            *
        //*  __| (_)_ _ ___ _ _| |_   __| |___ ___ __ _ _(_)_ __| |_ ___ _ _  *
        //* / _` | | '_/ -_) ' \  _| / _` / -_|_-</ _| '_| | '_ \  _/ _ \ '_| *
        //* \__,_|_|_| \___|_||_\__| \__,_\___/__/\__|_| |_| .__/\__\___/_|   *
        //*                                                |_|                *
        dirent const * PSTORE_NULLABLE
        dirent_descriptor::read () {
            return f_->read ();
        }
        void dirent_descriptor::rewind () { f_->rewind (); }


        //*                 __     *
        //*  _ _ ___ _ __  / _|___ *
        //* | '_/ _ \ '  \|  _(_-< *
        //* |_| \___/_|_|_|_| /__/ *
        //*                        *
        // ctor
        // ~~~~
        romfs::romfs (directory const * PSTORE_NONNULL root)
                : root_{root}
                , cwd_{root} {
            assert (this->fsck ());
        }

        // open
        // ~~~~
        auto romfs::open (gsl::czstring path) -> error_or<descriptor> {
            return this->parse_path (path) >>= [](dirent_ptr de) {
                auto file = std::make_shared<open_file> (*de);
                return error_or<descriptor>{descriptor{file}};
            };
        }

        // opendir
        // ~~~~~~~
        auto romfs::opendir (gsl::czstring path) -> error_or<dirent_descriptor> {
            auto get_directory = [](dirent_ptr de) -> error_or<directory const * PSTORE_NONNULL> {
                using rett = error_or<directory const * PSTORE_NONNULL>;
                return de->is_directory () ? rett{de->opendir ()}
                                           : rett{std::make_error_code (error_code::enotdir)};
            };

            auto create_descriptor = [](directory const * PSTORE_NONNULL d) {
                auto directory = std::make_shared<open_directory> (*d);
                return error_or<dirent_descriptor>{dirent_descriptor{directory}};
            };

            return (this->parse_path (path) >>= get_directory) >>= create_descriptor;
        }

        // getcwd
        // ~~~~~~
        error_or<std::string> romfs::getcwd () const { return dir_to_string (cwd_); }

        // chdir
        // ~~~~~
        std::error_code romfs::chdir (gsl::czstring path) {
            auto dirent_to_directory = [](dirent_ptr de) { return de->opendir (); };

            auto set_cwd = [this](directory const * const d) {
                cwd_ = d; // Warning: side-effect!
                return error_or<directory const *> (d);
            };

            auto const eo = (this->parse_path (path) >>= dirent_to_directory) >>= set_cwd;
            return eo.has_error () ? eo.get_error () : std::error_code{};
        }

        // directory_to_dirent [static]
        // ~~~~~~~~~~~~~~~~~~~
        dirent const * PSTORE_NONNULL
        romfs::directory_to_dirent (directory const * const PSTORE_NONNULL d) {
            auto * const r = d->find ("..");
            assert (r != nullptr && r->is_directory ());
            return static_cast<dirent const *> (r);
        }

        // parse_path
        // ~~~~~~~~~~
        auto romfs::parse_path (gsl::czstring path, directory const * dir) const
            -> error_or<dirent const *> {

            if (!path || !dir) {
                return error_or<dirent_ptr> (std::make_error_code (error_code::enoent));
            }

            gsl::czstring p = path;
            if (p[0] == '/') {
                dir = root_;
                p = next_component (path);
            }

            dirent const * PSTORE_NULLABLE current_de = directory_to_dirent (dir);
            assert (current_de->is_directory ());
            while (*p != '\0') {
                char const * component;
                char const * tail;
                std::tie (component, tail) = path_component (p);
                // note that component is not null terminated!
                p = next_component (tail);

                current_de = dir->find (component, tail - component);
                if (current_de == nullptr) {
                    // name not found
                    return error_or<dirent_ptr>{std::make_error_code (error_code::enoent)};
                }

                if (current_de->is_directory ()) {
                    error_or<directory const * PSTORE_NONNULL> eo_directory =
                        current_de->opendir ();
                    if (eo_directory.has_error ()) {
                        return error_or<dirent_ptr>{eo_directory.get_error ()};
                    }
                    dir = eo_directory.get_value ();
                } else if (*p != '\0') {
                    // not a directory and wasn't the last component.
                    return error_or<dirent_ptr> (std::make_error_code (error_code::enotdir));
                }
            }
            return error_or<dirent_ptr>{current_de};
        }

        // dir_to_string
        // ~~~~~~~~~~~~~
        error_or<std::string>
        romfs::dir_to_string (directory const * const PSTORE_NONNULL dir) const {
            if (dir == root_) {
                return error_or<std::string>{in_place, "/"};
            }
            dirent const * const parent_de = dir->find ("..");
            assert (parent_de != nullptr);
            return parent_de->opendir () >>= [=](directory const * const PSTORE_NONNULL parent) {
                return this->dir_to_string (parent) >>= [=](std::string s) {
                    assert (s.length () > 0);
                    if (s.back () != '/') {
                        s += '/';
                    }

                    dirent const * const p = parent->find (dir);
                    assert (p != nullptr);
                    return error_or<std::string> (s + p->name ());
                };
            };
        }

        // fsck
        // ~~~~
        bool romfs::fsck () const { return root_->check (root_); }

    } // end namespace romfs
} // end namespace pstore
