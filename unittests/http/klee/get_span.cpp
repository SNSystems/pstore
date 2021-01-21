//*             _                            *
//*   __ _  ___| |_   ___ _ __   __ _ _ __   *
//*  / _` |/ _ \ __| / __| '_ \ / _` | '_ \  *
//* | (_| |  __/ |_  \__ \ |_) | (_| | | | | *
//*  \__, |\___|\__| |___/ .__/ \__,_|_| |_| *
//*  |___/               |_|                 *
//===- unittests/http/klee/get_span.cpp -----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include <cstring>
#include <vector>

#include <klee/klee.h>

#include "pstore/http/buffered_reader.hpp"

using IO = int;
using byte_span = pstore::gsl::span<std::uint8_t>;


int main (int argc, char * argv[]) {

    auto refill = [] (IO io, byte_span const & sp) {
        std::memset (sp.data (), 0, sp.size ());
        return pstore::error_or_n<IO, byte_span::iterator>{pstore::in_place, io, sp.end ()};
    };

    std::size_t buffer_size;
    klee_make_symbolic (&buffer_size, sizeof (buffer_size), "buffer_size");
    std::size_t requested_size;
    klee_make_symbolic (&requested_size, sizeof (requested_size), "requested_size");

    klee_assume (buffer_size < 5);
    klee_assume (requested_size < 5);

    std::vector<std::uint8_t> v;
    v.resize (requested_size);

    auto io = IO{};
    auto br = pstore::httpd::make_buffered_reader<decltype (io)> (refill, buffer_size);
    br.get_span (io, pstore::gsl::make_span (v));
}
