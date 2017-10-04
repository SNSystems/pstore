//*   __                                      _    *
//*  / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                |___/                           *
//===- lib/pstore_mcrepo/fragment.cpp -------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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
#include "pstore_mcrepo/fragment.hpp"

using namespace pstore::repo;

//*             _   _           *
//*  ___ ___ __| |_(_)___ _ _   *
//* (_-</ -_) _|  _| / _ \ ' \  *
//* /__/\___\__|\__|_\___/_||_| *
//*                             *
// size_bytes
// ~~~~~~~~~~
std::size_t section::size_bytes (std::size_t data_size, std::size_t num_ifixups,
                                 std::size_t num_xfixups) {
    auto result = sizeof (section);
    result = section::part_size_bytes<std::uint8_t> (result, data_size);
    result = section::part_size_bytes<internal_fixup> (result, num_ifixups);
    result = section::part_size_bytes<external_fixup> (result, num_xfixups);
    return result;
}

std::size_t section::size_bytes () const {
    return section::size_bytes (data ().size (), ifixups ().size (), xfixups ().size ());
}

//*   __                             _    *
//*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_  *
//* |  _| '_/ _` / _` | '  \/ -_) ' \  _| *
//* |_| |_| \__,_\__, |_|_|_\___|_||_\__| *
//*              |___/                    *
// deleter::operator()
// ~~~~~~~~~~~~~~~~~~~
void fragment::deleter::operator() (void * p) {
    auto bytes = reinterpret_cast<std::uint8_t *> (p);
    delete[] bytes;
}

// operator[]
// ~~~~~~~~~~
section const & fragment::operator[] (section_type key) const {
    auto offset = arr_[static_cast<std::size_t> (key)];
    auto ptr = reinterpret_cast<std::uint8_t const *> (this) + offset;
    assert (reinterpret_cast<std::uintptr_t> (ptr) % alignof (section) == 0);
    return *reinterpret_cast<section const *> (ptr);
}

// eof: lib/pstore_mcrepo/fragment.cpp
