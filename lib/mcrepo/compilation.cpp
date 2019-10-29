//*                            _ _       _   _              *
//*   ___ ___  _ __ ___  _ __ (_) | __ _| |_(_) ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ \| | |/ _` | __| |/ _ \| '_ \  *
//* | (_| (_) | | | | | | |_) | | | (_| | |_| | (_) | | | | *
//*  \___\___/|_| |_| |_| .__/|_|_|\__,_|\__|_|\___/|_| |_| *
//*                     |_|                                 *
//===- lib/mcrepo/compilation.cpp -----------------------------------------===//
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
#include "pstore/mcrepo/compilation.hpp"

#include "pstore/mcrepo/repo_error.hpp"

using namespace pstore::repo;

std::ostream & pstore::repo::operator<< (std::ostream & os, linkage l) {
    char const * str = "unknown";
    switch (l) {
#define X(a)                                                                                       \
    case linkage::a: str = #a; break;
        PSTORE_REPO_LINKAGES
#undef X
    }
    return os << str;
}

constexpr std::array<char, 8> compilation::compilation_signature_;

// operator new
// ~~~~~~~~~~~~
void * compilation::operator new (std::size_t s, nmembers size) {
    (void) s;
    std::size_t const actual_bytes = compilation::size_bytes (size.n);
    assert (actual_bytes >= s);
    return ::operator new (actual_bytes);
}

void * compilation::operator new (std::size_t s, void * ptr) {
    return ::operator new (s, ptr);
}

// operator delete
// ~~~~~~~~~~~~~~~
void compilation::operator delete (void * p, nmembers /*size*/) {
    ::operator delete (p);
}

void compilation::operator delete (void * /*p*/, void * /*ptr*/) {}

void compilation::operator delete (void * p) {
    ::operator delete (p);
}

// load
// ~~~~
auto compilation::load (pstore::database const & db, pstore::extent<compilation> const & location)
    -> std::shared_ptr<compilation const> {
    std::shared_ptr<compilation const> t = db.getro (location);
#if PSTORE_SIGNATURE_CHECKS_ENABLED
    if (t->signature_ != compilation_signature_) {
        raise_error_code (make_error_code (error_code::bad_compilation_record));
    }
#endif
    if (t->size_bytes () != location.size) {
        raise_error_code (make_error_code (error_code::bad_compilation_record));
    }
    return t;
}
