//*  _                 _        _        *
//* (_) ___  ___   ___| |_ __ _| |_ ___  *
//* | |/ _ \/ __| / __| __/ _` | __/ _ \ *
//* | | (_) \__ \ \__ \ || (_| | ||  __/ *
//* |_|\___/|___/ |___/\__\__,_|\__\___| *
//*                                      *
//===- include/pstore/support/ios_state.hpp -------------------------------===//
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
/// \file ios_state.hpp
/// \brief Save and restore of stream flags.

#ifndef PSTORE_SUPPORT_IOS_STATE_HPP
#define PSTORE_SUPPORT_IOS_STATE_HPP

#include <iosfwd>

#include "pstore/support/portab.hpp"

namespace pstore {

    /// \brief A class used to save an iostream's formatting flags on construction
    /// and restore them on destruction.
    ///
    /// Normally used to manage the restoration of the flags on exit from a scope.
    class ios_flags_saver {
    public:
        explicit ios_flags_saver (std::ios_base & stream)
                : ios_flags_saver (stream, stream.flags ()) {}

        ios_flags_saver (std::ios_base & stream, std::ios_base::fmtflags const & flags) noexcept
                : stream_{stream}
                , flags_{flags} {}
        // No copying, moving, or assignment.
        ios_flags_saver (ios_flags_saver const &) = delete;
        ios_flags_saver (ios_flags_saver && rhs) noexcept = delete;

        ~ios_flags_saver () noexcept {
            PSTORE_NO_EX_ESCAPE ({ stream_.flags (flags_); });
        }

        // No copying, moving, or assignment.
        ios_flags_saver & operator= (ios_flags_saver const &) = delete;
        ios_flags_saver & operator= (ios_flags_saver && rhs) = delete;

    private:
        std::ios_base & stream_;
        std::ios_base::fmtflags flags_;
    };

} // namespace pstore

#endif // PSTORE_SUPPORT_IOS_STATE_HPP
