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
//===- lib/os/memory_mapper.cpp -------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file memory_mapper.cpp

#include "pstore/os/memory_mapper.hpp"
#include <ostream>

namespace pstore {

    system_page_size_interface::~system_page_size_interface () noexcept = default;
    system_page_size::~system_page_size () noexcept = default;

    void memory_mapper_base::read_only (void * const addr, std::size_t const len) {
#ifndef NDEBUG
        {
            auto * const addr8 = static_cast<std::uint8_t *> (addr);
            auto * const data8 = static_cast<std::uint8_t *> (this->data ().get ());
            PSTORE_ASSERT (addr8 >= data8 && addr8 + len <= data8 + this->size ());
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
