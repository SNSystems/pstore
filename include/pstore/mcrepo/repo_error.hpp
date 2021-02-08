//*                                                   *
//*  _ __ ___ _ __   ___     ___ _ __ _ __ ___  _ __  *
//* | '__/ _ \ '_ \ / _ \   / _ \ '__| '__/ _ \| '__| *
//* | | |  __/ |_) | (_) | |  __/ |  | | | (_) | |    *
//* |_|  \___| .__/ \___/   \___|_|  |_|  \___/|_|    *
//*          |_|                                      *
//===- include/pstore/mcrepo/repo_error.hpp -------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_MCREPO_REPO_ERROR_HPP
#define PSTORE_MCREPO_REPO_ERROR_HPP

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

        class error_category final : public std::error_category {
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

#endif // PSTORE_MCREPO_REPO_ERROR_HPP
