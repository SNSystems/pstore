//*                                             *
//*  _ __ ___   ___ _ __ ___   ___  _ __ _   _  *
//* | '_ ` _ \ / _ \ '_ ` _ \ / _ \| '__| | | | *
//* | | | | | |  __/ | | | | | (_) | |  | |_| | *
//* |_| |_| |_|\___|_| |_| |_|\___/|_|   \__, | *
//*                                      |___/  *
//*                                         *
//*  _ __ ___   __ _ _ __  _ __   ___ _ __  *
//* | '_ ` _ \ / _` | '_ \| '_ \ / _ \ '__| *
//* | | | | | | (_| | |_) | |_) |  __/ |    *
//* |_| |_| |_|\__,_| .__/| .__/ \___|_|    *
//*                 |_|   |_|               *
//===- lib/core/memory_mapper.cpp -----------------------------------------===//
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
/// \file memory_mapper.cpp

#include "pstore/core/memory_mapper.hpp"
#include <ostream>

namespace pstore {

    system_page_size_interface::~system_page_size_interface () noexcept = default;
    system_page_size::~system_page_size () noexcept = default;

    void memory_mapper_base::read_only (void * addr, std::size_t len) {
#ifndef NDEBUG
        {
            auto addr8 = static_cast<std::uint8_t *> (addr);
            auto data8 = static_cast<std::uint8_t *> (this->data ().get ());
            assert (addr8 >= data8 && addr8 + len <= data8 + this->size ());
        }
#endif
        this->read_only_impl (addr, len);
    }


    // (dtor)
    // ~~~~~~
    memory_mapper_base::~memory_mapper_base () = default;

    // page_size [static]
    // ~~~~~~~~~
    unsigned long memory_mapper_base::page_size (system_page_size_interface const & ps) {
        return ps.get ();
    }


    std::ostream & operator<< (std::ostream & os, memory_mapper_base const & mm) {
        return os << "{ offset: " << mm.offset () << ", size: " << mm.size () << " }";
    }

    in_memory_mapper::~in_memory_mapper () noexcept = default;

} // namespace pstore
