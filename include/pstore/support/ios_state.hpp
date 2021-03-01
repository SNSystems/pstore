//===- include/pstore/support/ios_state.hpp ---------------*- mode: C++ -*-===//
//*  _                 _        _        *
//* (_) ___  ___   ___| |_ __ _| |_ ___  *
//* | |/ _ \/ __| / __| __/ _` | __/ _ \ *
//* | | (_) \__ \ \__ \ || (_| | ||  __/ *
//* |_|\___/|___/ |___/\__\__,_|\__\___| *
//*                                      *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
            no_ex_escape ([this] () { stream_.flags (flags_); });
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
