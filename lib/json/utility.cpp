//*        _   _ _ _ _          *
//*  _   _| |_(_) (_) |_ _   _  *
//* | | | | __| | | | __| | | | *
//* | |_| | |_| | | | |_| |_| | *
//*  \__,_|\__|_|_|_|\__|\__, | *
//*                      |___/  *
//===- lib/json/utility.cpp -----------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/json/utility.hpp"

#include "pstore/json/dom_types.hpp"
#include "pstore/json/json.hpp"

namespace pstore {
    namespace json {

        bool is_valid (std::string const & str) {
            pstore::json::parser<pstore::json::null_output> p;
            p.input (pstore::gsl::make_span (str));
            p.eof ();
            return !p.has_error ();
        }

    } // end namespace json
} // end namespace pstore
