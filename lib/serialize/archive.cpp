//===- lib/serialize/archive.cpp ------------------------------------------===//
//*                 _     _            *
//*   __ _ _ __ ___| |__ (_)_   _____  *
//*  / _` | '__/ __| '_ \| \ \ / / _ \ *
//* | (_| | | | (__| | | | |\ V /  __/ *
//*  \__,_|_|  \___|_| |_|_| \_/ \___| *
//*                                    *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file serialize/archive.cpp
/// \brief Implementation of the basic archive reader and writer types.

#include "pstore/serialize/archive.hpp"

#include <iomanip>
#include <ostream>

#include "pstore/support/ios_state.hpp"

namespace {
    template <typename InputIterator>
    std::ostream & hex_dump (std::ostream & os, InputIterator first, InputIterator last) {
        pstore::ios_flags_saver const flags{os};
        os << std::setfill ('0') << std::hex;

        using value_type = typename std::iterator_traits<InputIterator>::value_type;
        char const * separator = "";
        std::for_each (first, last, [&] (value_type v) {
            PSTORE_ASSERT (v >= 0 && v <= 0xff);
            os << separator << std::setw (2) << unsigned{v};
            separator = " ";
        });
        return os;
    }
} // end anonymous namespace


namespace pstore {
    namespace serialize {
        namespace archive {

            // ****
            // null
            // ****
            null::~null () noexcept = default;


            // *************
            // vector_writer
            // *************

            vector_writer::~vector_writer () noexcept = default;

            std::ostream & operator<< (std::ostream & os, vector_writer const & writer) {
                return hex_dump (os, std::begin (writer), std::end (writer));
            }


            // *************
            // buffer_writer
            // *************

            buffer_writer::~buffer_writer () noexcept = default;

            std::ostream & operator<< (std::ostream & os, buffer_writer const & writer) {
                return hex_dump (os, std::begin (writer), std::end (writer));
            }

        } // namespace archive
    }     // namespace serialize
} // namespace pstore
