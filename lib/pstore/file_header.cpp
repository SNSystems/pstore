//*   __ _ _        _                    _            *
//*  / _(_) | ___  | |__   ___  __ _  __| | ___ _ __  *
//* | |_| | |/ _ \ | '_ \ / _ \/ _` |/ _` |/ _ \ '__| *
//* |  _| | |  __/ | | | |  __/ (_| | (_| |  __/ |    *
//* |_| |_|_|\___| |_| |_|\___|\__,_|\__,_|\___|_|    *
//*                                                   *
//===- lib/pstore/file_header.cpp -----------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
/// \file file_header.cpp
/// \brief Implementations of the file header and footer member functions.

#include "pstore/file_header.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iterator>
#include <random>
#include <set>
#include <utility>

// pstore includes
#include "pstore/crc32.hpp"
#include "pstore/database.hpp"

// Local includes
#include "config.hpp"


namespace pstore {

    std::uint16_t const header::major_version;
    std::uint16_t const header::minor_version;
    std::array<std::uint8_t, 4> const header::file_signature1{{
        std::uint8_t ('p'), std::uint8_t ('S'), std::uint8_t ('t'), std::uint8_t ('r'),
    }};
    std::uint32_t const header::file_signature2;


    // header
    // ~~~~~~
    header::header ()
            : footer_pos () {

        PSTORE_STATIC_ASSERT (sizeof (file_signature1[0]) == sizeof (a.signature1[0]));
        PSTORE_STATIC_ASSERT (sizeof (file_signature1) == sizeof (a.signature1));
        assert (footer_pos.is_lock_free ());

        a.signature1 = file_signature1;
        a.signature2 = file_signature2;
        a.version[0] = header::major_version;
        a.version[1] = header::minor_version;
        crc = this->get_crc ();
    }

    // is_valid
    // ~~~~~~~~
    bool header::is_valid () const noexcept {
#if PSTORE_CRC_CHECKS_ENABLED
        return crc == this->get_crc ();
#else
        return true;
#endif
    }

    // get_crc
    // ~~~~~~~
    std::uint32_t header::get_crc () const noexcept {
        return crc32 (::pstore::gsl::make_span (&this->a, 1));
    }
} // namespace pstore

namespace pstore {
    std::array<std::uint8_t, 8> trailer::default_signature1{
        {'h', 'P', 'P', 'y', 'f', 'o', 'o', 'T'}};
    std::array<std::uint8_t, 8> trailer::default_signature2{
        {'h', 'P', 'P', 'y', 'T', 'a', 'i', 'l'}};

    // crc_is_valid
    // ~~~~~~~~~~~~
    bool trailer::crc_is_valid () const {
#if PSTORE_CRC_CHECKS_ENABLED
        return crc == this->get_crc ();
#else
        return true;
#endif
    }

    // get_crc
    // ~~~~~~~
    std::uint32_t trailer::get_crc () const noexcept {
        return crc32 (::pstore::gsl::make_span (&this->a, 1));
    }

    // validate [static]
    // ~~~~~~~~
    bool trailer::validate (database const & db, address pos) {
        if (pos == address::null ()) {
            return true;
        }
        bool ok = true;
        // A basic validity check of footer_pos and prev before we go and
        // access them.
        if (pos < address::make (sizeof (header)) || pos.absolute () % alignof (trailer) != 0) {
            ok = false;
        } else {
            auto const footer = db.getro<trailer> (pos);
            // Get the address of the previous generation.
            address prev_pos = footer->a.prev_generation;

            if (!footer->crc_is_valid () || footer->a.signature1 != trailer::default_signature1 ||
                footer->signature2 != trailer::default_signature2) {

                ok = false;
            } else if (pos >= address::make (sizeof (trailer)) &&
                       prev_pos > pos - sizeof (trailer)) {
                // The previous trailer must lie before the current one in the file,
                // be separated by at least the size of the trailer and agree with the location
                // given by the current trailer's 'size' field.
                ok = false;
            } else if (pos.absolute () < footer->a.size) {
                ok = false;
            } else {
                auto const transaction_first_byte = prev_pos == address::null ()
                                                        ? address::make (sizeof (header))
                                                        : prev_pos + sizeof (trailer);
                if (pos - footer->a.size != transaction_first_byte) {
                    ok = false;
                }
            }
        }

        if (!ok) {
            raise (error_code::footer_corrupt, db.path ());
        }
        return ok;
    }
} // namespace pstore
// eof: lib/pstore/file_header.cpp
