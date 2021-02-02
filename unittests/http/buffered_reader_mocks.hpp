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
//===- unittests/http/buffered_reader_mocks.hpp ---------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef BUFFERED_READER_MOCKS_HPP
#define BUFFERED_READER_MOCKS_HPP

#include <functional>
#include <string>
#include <utility>

#include <gmock/gmock.h>

#include "pstore/adt/error_or.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/maybe.hpp"

using getc_result_type = pstore::error_or_n<int, pstore::maybe<char>>;
using gets_result_type = pstore::error_or_n<int, pstore::maybe<std::string>>;

using refiller_result_type = pstore::error_or_n<int, pstore::gsl::span<std::uint8_t>::iterator>;
using refiller_function =
    std::function<refiller_result_type (int, pstore::gsl::span<std::uint8_t> const &)>;

class mock_refiller {
public:
    virtual ~mock_refiller () noexcept;
    virtual refiller_result_type fill (int c, pstore::gsl::span<std::uint8_t> const &) const = 0;
};

class refiller : public mock_refiller {
public:
    ~refiller () noexcept override;

    MOCK_CONST_METHOD2 (fill, refiller_result_type (int, pstore::gsl::span<std::uint8_t> const &));
    refiller_function refill_function () const;
};

/// Returns a function which which simply returns end-of-stream when invoked.
refiller_function eof ();

refiller_function yield_bytes (pstore::gsl::span<std::uint8_t const> const & v);

/// Returns a function which will yield the string passed as its argument.
refiller_function yield_string (std::string const & str);

#endif // BUFFERED_READER_MOCKS_HPP
