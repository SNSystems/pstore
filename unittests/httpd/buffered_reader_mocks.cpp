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
//===- unittests/httpd/buffered_reader_mocks.cpp --------------------------===//
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

#include "buffered_reader_mocks.hpp"

refiller_function refiller::refill_function () const {
    return [this](int io, pstore::gsl::span<char> const & s) { return this->fill (io, s); };
}

/// Returns a function which which simply returns end-of-stream when invoked.
refiller_function eof () {
    return [](int io, pstore::gsl::span<char> const & s) {
        return refiller_result_type{pstore::in_place, io + 1, s.begin ()};
    };
}

/// Returns a function which will yield the string passed as its argument.
refiller_function yield_string (std::string const & str) {
    return [str](int io, pstore::gsl::span<char> const & s) {
#ifndef DEBUG
        auto const size = s.size ();
        assert (size > 0 && str.length () <=
                                static_cast<std::make_unsigned<decltype (size)>::type> (s.size ()));
#endif
        return refiller_result_type{pstore::in_place, io + 1,
                                    std::copy (str.begin (), str.end (), s.begin ())};
    };
}
