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
#ifndef BUFFERED_READER_MOCKS_HPP
#define BUFFERED_READER_MOCKS_HPP

#include <functional>
#include <string>
#include <utility>

#include <gmock/gmock.h>

#include "pstore/adt/error_or.hpp"
#include "pstore/adt/maybe.hpp"
#include "pstore/support/gsl.hpp"

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
