//===- include/pstore/http/media_type.hpp -----------------*- mode: C++ -*-===//
//*                     _ _         _                     *
//*  _ __ ___   ___  __| (_) __ _  | |_ _   _ _ __   ___  *
//* | '_ ` _ \ / _ \/ _` | |/ _` | | __| | | | '_ \ / _ \ *
//* | | | | | |  __/ (_| | | (_| | | |_| |_| | |_) |  __/ *
//* |_| |_| |_|\___|\__,_|_|\__,_|  \__|\__, | .__/ \___| *
//*                                     |___/|_|          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_HTTP_MEDIA_TYPE_HPP
#define PSTORE_HTTP_MEDIA_TYPE_HPP

#include <string>

#include "pstore/support/gsl.hpp"
#include "pstore/support/portab.hpp"

namespace pstore {
    namespace http {

        gsl::czstring PSTORE_NONNULL
        media_type_from_extension (gsl::czstring PSTORE_NONNULL extension);

        gsl::czstring PSTORE_NONNULL media_type_from_filename (std::string const & filename);

    } // end namespace http
} // end namespace pstore

#endif // PSTORE_HTTP_MEDIA_TYPE_HPP
