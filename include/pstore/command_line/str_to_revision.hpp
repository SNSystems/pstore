//*      _          _                         _     _              *
//*  ___| |_ _ __  | |_ ___    _ __ _____   _(_)___(_) ___  _ __   *
//* / __| __| '__| | __/ _ \  | '__/ _ \ \ / / / __| |/ _ \| '_ \  *
//* \__ \ |_| |    | || (_) | | | |  __/\ V /| \__ \ | (_) | | | | *
//* |___/\__|_|     \__\___/  |_|  \___| \_/ |_|___/_|\___/|_| |_| *
//*                                                                *
//===- include/pstore/command_line/str_to_revision.hpp --------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file command_line/str_to_revision.hpp
/// \brief Converts a string to a pair containing a revision number and boolean indicating whether
/// the conversion was successful.

#ifndef PSTORE_COMMAND_LINE_STR_TO_REVISION_HPP
#define PSTORE_COMMAND_LINE_STR_TO_REVISION_HPP

#include <string>
#include <utility>

#include "pstore/support/maybe.hpp"

namespace pstore {

    /// Converts a string to a revision number. Leading and trailing whitespace is ignored, the text
    /// "head" (regardless of case) will become pstore::database::head_revision.
    ///
    /// \param str  The string to be converted to a revision number.
    /// \returns Either the converted revision number or nothing.

    maybe<unsigned> str_to_revision (std::string const & str);

} // end namespace pstore

#endif // PSTORE_COMMAND_LINE_STR_TO_REVISION_HPP
