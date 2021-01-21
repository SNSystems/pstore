//*            _     _                    *
//*   __ _  __| | __| |_ __ ___  ___ ___  *
//*  / _` |/ _` |/ _` | '__/ _ \/ __/ __| *
//* | (_| | (_| | (_| | | |  __/\__ \__ \ *
//*  \__,_|\__,_|\__,_|_|  \___||___/___/ *
//*                                       *
//===- lib/core/address.cpp -----------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file address.cpp

#include "pstore/core/address.hpp"
#include <ostream>

namespace pstore {

    constexpr unsigned const address::offset_number_bits;
    constexpr unsigned const address::segment_number_bits;

    constexpr address::offset_type const address::max_offset;
    constexpr address::segment_type const address::max_segment;

    constexpr std::uint64_t const address::segment_size;

    std::ostream & operator<< (std::ostream & os, address const & addr) {
        return os << "{s:" << addr.segment () << " +:" << addr.offset () << "}";
    }

} // namespace pstore
