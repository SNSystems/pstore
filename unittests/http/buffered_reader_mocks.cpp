//===- unittests/http/buffered_reader_mocks.cpp ---------------------------===//
//*  _            __  __                   _                      _            *
//* | |__  _   _ / _|/ _| ___ _ __ ___  __| |  _ __ ___  __ _  __| | ___ _ __  *
//* | '_ \| | | | |_| |_ / _ \ '__/ _ \/ _` | | '__/ _ \/ _` |/ _` |/ _ \ '__| *
//* | |_) | |_| |  _|  _|  __/ | |  __/ (_| | | | |  __/ (_| | (_| |  __/ |    *
//* |_.__/ \__,_|_| |_|  \___|_|  \___|\__,_| |_|  \___|\__,_|\__,_|\___|_|    *
//*                                                                            *
//*                       _         *
//*  _ __ ___   ___   ___| | _____  *
//* | '_ ` _ \ / _ \ / __| |/ / __| *
//* | | | | | | (_) | (__|   <\__ \ *
//* |_| |_| |_|\___/ \___|_|\_\___/ *
//*                                 *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "buffered_reader_mocks.hpp"

#include <algorithm>
#include <vector>

#include "pstore/support/unsigned_cast.hpp"

mock_refiller::~mock_refiller () noexcept = default;

refiller::~refiller () noexcept = default;


refiller_function refiller::refill_function () const {
    return
        [this] (int io, pstore::gsl::span<std::uint8_t> const & s) { return this->fill (io, s); };
}

/// Returns a function which which simply returns end-of-stream when invoked.
refiller_function eof () {
    return [] (int io, pstore::gsl::span<std::uint8_t> const & s) {
        return refiller_result_type{pstore::in_place, io + 1, s.begin ()};
    };
}

/// Returns a function which will yield the bytes as its argument.
refiller_function yield_bytes (pstore::gsl::span<std::uint8_t const> const & v) {
    return [v] (int io, pstore::gsl::span<std::uint8_t> const & s) {
        PSTORE_ASSERT (s.size () > 0 && v.size () <= s.size ());
        return refiller_result_type{pstore::in_place, io + 1,
                                    std::copy (v.begin (), v.end (), s.begin ())};
    };
}

/// Returns a function which will yield the string passed as its argument.
refiller_function yield_string (std::string const & str) {
    return [str] (int io, pstore::gsl::span<std::uint8_t> const & s) {
        PSTORE_ASSERT (str.length () <= pstore::unsigned_cast (s.size ()));
        return refiller_result_type{
            pstore::in_place, io + 1,
            std::transform (str.begin (), str.end (), s.begin (), [] (std::uint8_t v) {
                PSTORE_ASSERT (v < 128);
                return static_cast<char> (v);
            })};
    };
}
