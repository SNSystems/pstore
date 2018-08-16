//*                                  _   _              *
//*   __ _  ___ _ __   ___ _ __ __ _| |_(_) ___  _ __   *
//*  / _` |/ _ \ '_ \ / _ \ '__/ _` | __| |/ _ \| '_ \  *
//* | (_| |  __/ | | |  __/ | | (_| | |_| | (_) | | | | *
//*  \__, |\___|_| |_|\___|_|  \__,_|\__|_|\___/|_| |_| *
//*  |___/                                              *
//*  _ _                 _              *
//* (_) |_ ___ _ __ __ _| |_ ___  _ __  *
//* | | __/ _ \ '__/ _` | __/ _ \| '__| *
//* | | ||  __/ | | (_| | || (_) | |    *
//* |_|\__\___|_|  \__,_|\__\___/|_|    *
//*                                     *
//===- include/pstore/core/generation_iterator.hpp ------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
/// \file generation_iterator.hpp

#ifndef PSTORE_GENERATION_ITERATOR_HPP
#define PSTORE_GENERATION_ITERATOR_HPP (1)

#include <iterator>
#include <tuple>

#include "pstore/core/file_header.hpp"

namespace pstore {
    class database;
    struct trailer;

    class generation_iterator : public std::iterator<std::input_iterator_tag, // category
                                                     typed_address<trailer>> {
    public:
        generation_iterator (database const & db, typed_address<trailer> pos)
                : db_ (db)
                , pos_ (pos) {

            this->validate ();
        }
        generation_iterator (generation_iterator &&) = default;
        generation_iterator & operator= (generation_iterator &&) = default;
        generation_iterator (generation_iterator const &) = default;
        generation_iterator & operator= (generation_iterator const &) = default;

        bool operator== (generation_iterator const & rhs) const {
            database const * const ldb = &db_;
            database const * const rdb = &rhs.db_;
            return std::tie (ldb, pos_) == std::tie (rdb, rhs.pos_);
        }
        bool operator!= (generation_iterator const & rhs) const { return !operator== (rhs); }

        typed_address<trailer> operator* () const { return pos_; }

        generation_iterator & operator++ ();  // pre-increment
        generation_iterator operator++ (int); // post-increment

    private:
        bool validate () const;

        database const & db_;
        typed_address<trailer> pos_;
    };


    class generation_container {
    public:
        explicit generation_container (database const & db)
                : db_ (db) {}
        generation_iterator begin ();
        generation_iterator end ();

    private:
        database const & db_;
    };

} // end namespace pstore
#endif // PSTORE_GENERATION_ITERATOR_HPP
