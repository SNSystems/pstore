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
//===- lib/core/generation_iterator.cpp -----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file generation_iterator.cpp

#include "pstore/core/generation_iterator.hpp"

#include "pstore/core/database.hpp"

namespace pstore {
    // ***********************
    // * generation iterator *
    // ***********************
    // (pre-increment)
    // ~~~~~~~~~~~~~~~
    generation_iterator & generation_iterator::operator++ () {
        pos_ = db_->getro<pstore::trailer> (pos_)->a.prev_generation;
        this->validate ();
        return *this;
    }

    // (post-increment)
    // ~~~~~~~~~~~~~~~~
    generation_iterator generation_iterator::operator++ (int) {
        generation_iterator prev = *this;
        ++(*this);
        return prev;
    }

    // validate
    // ~~~~~~~~
    bool generation_iterator::validate () const { return trailer::validate (*db_, pos_); }


    // ************************
    // * generation container *
    // ************************
    generation_iterator generation_container::begin () { return {&db_, db_.footer_pos ()}; }
    generation_iterator generation_container::end () {
        return {&db_, pstore::typed_address<pstore::trailer>::null ()};
    }

} // namespace pstore
