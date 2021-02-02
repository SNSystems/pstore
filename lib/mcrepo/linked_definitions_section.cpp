//*  _ _       _            _       _       __ _       _ _   _                  *
//* | (_)_ __ | | _____  __| |   __| | ___ / _(_)_ __ (_) |_(_) ___  _ __  ___  *
//* | | | '_ \| |/ / _ \/ _` |  / _` |/ _ \ |_| | '_ \| | __| |/ _ \| '_ \/ __| *
//* | | | | | |   <  __/ (_| | | (_| |  __/  _| | | | | | |_| | (_) | | | \__ \ *
//* |_|_|_| |_|_|\_\___|\__,_|  \__,_|\___|_| |_|_| |_|_|\__|_|\___/|_| |_|___/ *
//*                                                                             *
//*                _   _              *
//*  ___  ___  ___| |_(_) ___  _ __   *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* \__ \  __/ (__| |_| | (_) | | | | *
//* |___/\___|\___|\__|_|\___/|_| |_| *
//*                                   *
//===- lib/mcrepo/linked_definitions_section.cpp --------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/mcrepo/linked_definitions_section.hpp"

namespace pstore {
    namespace repo {

        //*           _            _                   *
        //* __ ____ _| |_  _ ___  | |_ _  _ _ __  ___  *
        //* \ V / _` | | || / -_) |  _| || | '_ \/ -_) *
        //*  \_/\__,_|_|\_,_\___|  \__|\_, | .__/\___| *
        //*                            |__/|_|         *

        // operator==
        // ~~~~~~~~~~
        bool linked_definitions::value_type::operator== (value_type const & rhs) const noexcept {
            if (&rhs == this) {
                return true;
            }
            return compilation == rhs.compilation && index == rhs.index && pointer == rhs.pointer;
        }


        //*  _ _      _          _      _      __ _      _ _   _              *
        //* | (_)_ _ | |_____ __| |  __| |___ / _(_)_ _ (_) |_(_)___ _ _  ___ *
        //* | | | ' \| / / -_) _` | / _` / -_)  _| | ' \| |  _| / _ \ ' \(_-< *
        //* |_|_|_||_|_\_\___\__,_| \__,_\___|_| |_|_||_|_|\__|_\___/_||_/__/ *
        //*                                                                   *

        // size bytes
        // ~~~~~~~~~~
        std::size_t linked_definitions::size_bytes (std::uint64_t const size) noexcept {
            return sizeof (linked_definitions) - sizeof (definitions_) +
                   sizeof (definitions_[0]) * size;
        }

        std::size_t linked_definitions::size_bytes () const noexcept {
            return size_bytes (this->size ());
        }

        // load
        // ~~~~
        std::shared_ptr<linked_definitions const>
        linked_definitions::load (database const & db, typed_address<linked_definitions> const ld) {
            // First work out its size, then read the full-size of the object.
            std::shared_ptr<linked_definitions const> const ln = db.getro (ld);
            return db.getro (ld, linked_definitions::size_bytes (ln->size ()));
        }

        // operator<<
        // ~~~~~~~~~~
        std::ostream & operator<< (std::ostream & os, linked_definitions::value_type const & ld) {
            return os << "{compilation:" << ld.compilation << ", index:" << ld.index
                      << ", unused:" << ld.unused << ", pointer:" << ld.pointer << '}';
        }

        //*                  _   _               _ _               _      _             *
        //*  __ _ _ ___ __ _| |_(_)___ _ _    __| (_)____ __  __ _| |_ __| |_  ___ _ _  *
        //* / _| '_/ -_) _` |  _| / _ \ ' \  / _` | (_-< '_ \/ _` |  _/ _| ' \/ -_) '_| *
        //* \__|_| \___\__,_|\__|_\___/_||_| \__,_|_/__/ .__/\__,_|\__\__|_||_\___|_|   *
        //*                                            |_|                              *

        // size bytes
        // ~~~~~~~~~~
        std::size_t linked_definitions_creation_dispatcher::size_bytes () const {
            static_assert (sizeof (std::uint64_t) >= sizeof (std::uintptr_t),
                           "sizeof uint64_t should be at least sizeof uintptr_t");
            PSTORE_ASSERT (std::distance (begin_, end_) >= 0);
            return linked_definitions::size_bytes (
                static_cast<std::uint64_t> (std::distance (begin_, end_)));
        }

        // write
        // ~~~~~
        std::uint8_t *
        linked_definitions_creation_dispatcher::write (std::uint8_t * const out) const {
            PSTORE_ASSERT (this->aligned (out) == out);
            auto * const dependent = new (out) linked_definitions (begin_, end_);
            return out + dependent->size_bytes ();
        }

        // aligned impl
        // ~~~~~~~~~~~~
        std::uintptr_t
        linked_definitions_creation_dispatcher::aligned_impl (std::uintptr_t const in) const {
            return pstore::aligned<linked_definitions> (in);
        }


        //*     _ _               _      _             *
        //*  __| (_)____ __  __ _| |_ __| |_  ___ _ _  *
        //* / _` | (_-< '_ \/ _` |  _/ _| ' \/ -_) '_| *
        //* \__,_|_/__/ .__/\__,_|\__\__|_||_\___|_|   *
        //*           |_|                              *
        linked_definitions_dispatcher::~linked_definitions_dispatcher () noexcept = default;

        PSTORE_NO_RETURN void linked_definitions_dispatcher::error () const {
            pstore::raise_error_code (make_error_code (error_code::bad_fragment_type));
        }

    } // end namespace repo
} // end namespace pstore
