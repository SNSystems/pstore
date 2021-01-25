//*                    _           _  *
//*   __ _ _   _  ___ | |_ ___  __| | *
//*  / _` | | | |/ _ \| __/ _ \/ _` | *
//* | (_| | |_| | (_) | ||  __/ (_| | *
//*  \__, |\__,_|\___/ \__\___|\__,_| *
//*     |_|                           *
//===- include/pstore/support/quoted.hpp ----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file quoted.hpp
/// \brief Wrapping quotation marks around strings for output to the user.

#ifndef PSTORE_SUPPORT_QUOTED_HPP
#define PSTORE_SUPPORT_QUOTED_HPP

#include <iomanip>

#include "pstore/support/gsl.hpp"

namespace pstore {

    /// Wraps quotation marks around a string for presentation to the user. Intended for use
    /// as an io-manipulator.
    ///
    /// \param str The string to be wrapped in quotation marks.
    inline auto quoted (gsl::czstring const str) { return std::quoted (str, '"', '\0'); }

    /// Wraps quotation marks around a string for presentation to the user. Intended for use
    /// as an io-manipulator.
    ///
    /// \param str The string to be wrapped in quotation marks.
    inline auto quoted (std::string const & str) { return std::quoted (str, '"', '\0'); }

} // end namespace pstore

#endif // PSTORE_SUPPORT_QUOTED_HPP
