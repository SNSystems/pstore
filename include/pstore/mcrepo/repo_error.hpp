//*                                                   *
//*  _ __ ___ _ __   ___     ___ _ __ _ __ ___  _ __  *
//* | '__/ _ \ '_ \ / _ \   / _ \ '__| '__/ _ \| '__| *
//* | | |  __/ |_) | (_) | |  __/ |  | | | (_) | |    *
//* |_|  \___| .__/ \___/   \___|_|  |_|  \___/|_|    *
//*          |_|                                      *
//===- include/pstore/mcrepo/repo_error.hpp -------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_MCREPO_ERROR_HPP
#define PSTORE_MCREPO_ERROR_HPP

#include <string>
#include <system_error>
#include <type_traits>

#include "pstore/support/error.hpp"

namespace pstore {
    namespace repo {

        enum class error_code : int {
            bad_fragment_record = 1,
            bad_fragment_type, // an attempt to get an unavailable fragment type
            bad_compilation_record,
            too_many_members_in_compilation,
            bss_section_too_large,
        };

        class error_category : public std::error_category {
        public:
            // The need for this constructor was removed by CWG defect 253 but Clang (prior
            // to 3.9.0) and GCC (before 4.6.4) require its presence.
            error_category () noexcept {}
            char const * name () const noexcept override;
            std::string message (int error) const override;
        };

        std::error_code make_error_code (error_code e);

    } // end namespace repo
} // end namespace pstore

namespace std {

    template <>
    struct is_error_code_enum<pstore::repo::error_code> : true_type {};

} // end namespace std

#endif // PSTORE_MCREPO_ERROR_HPP
