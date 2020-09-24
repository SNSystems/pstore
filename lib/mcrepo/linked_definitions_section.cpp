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
#include "pstore/mcrepo/linked_definitions_section.hpp"

namespace pstore {
    namespace repo {

        //*  _ _      _          _      _      __ _      _ _   _              *
        //* | (_)_ _ | |_____ __| |  __| |___ / _(_)_ _ (_) |_(_)___ _ _  ___ *
        //* | | | ' \| / / -_) _` | / _` / -_)  _| | ' \| |  _| / _ \ ' \(_-< *
        //* |_|_|_||_|_\_\___\__,_| \__,_\___|_| |_|_||_|_|\__|_\___/_||_/__/ *
        //*                                                                   *
        // size_bytes
        // ~~~~~~~~~~
        std::size_t linked_definitions::size_bytes (std::uint64_t const size) noexcept {
            if (size == 0) {
                return 0;
            }
            return sizeof (linked_definitions) - sizeof (linked_definitions::compilation_members_) +
                   sizeof (linked_definitions::compilation_members_[0]) * size;
        }

        // size_bytes
        // ~~~~~~~~~~
        /// Returns the number of bytes of storage required for the linked definitions.
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


        //*                  _   _               _ _               _      _             *
        //*  __ _ _ ___ __ _| |_(_)___ _ _    __| (_)____ __  __ _| |_ __| |_  ___ _ _  *
        //* / _| '_/ -_) _` |  _| / _ \ ' \  / _` | (_-< '_ \/ _` |  _/ _| ' \/ -_) '_| *
        //* \__|_| \___\__,_|\__|_\___/_||_| \__,_|_/__/ .__/\__,_|\__\__|_||_\___|_|   *
        //*                                            |_|                              *
        std::size_t linked_definitions_creation_dispatcher::size_bytes () const {
            static_assert (sizeof (std::uint64_t) >= sizeof (std::uintptr_t),
                           "sizeof uint64_t should be at least sizeof uintptr_t");
            return linked_definitions::size_bytes (static_cast<std::uint64_t> (end_ - begin_));
        }

        std::uint8_t *
        linked_definitions_creation_dispatcher::write (std::uint8_t * const out) const {
            assert (this->aligned (out) == out);
            if (begin_ == end_) {
                return out;
            }
            auto * const dependent = new (out) linked_definitions (begin_, end_);
            return out + dependent->size_bytes ();
        }

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
