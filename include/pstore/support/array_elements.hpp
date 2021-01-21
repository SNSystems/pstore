//*                                    _                           _        *
//*   __ _ _ __ _ __ __ _ _   _    ___| | ___ _ __ ___   ___ _ __ | |_ ___  *
//*  / _` | '__| '__/ _` | | | |  / _ \ |/ _ \ '_ ` _ \ / _ \ '_ \| __/ __| *
//* | (_| | |  | | | (_| | |_| | |  __/ |  __/ | | | | |  __/ | | | |_\__ \ *
//*  \__,_|_|  |_|  \__,_|\__, |  \___|_|\___|_| |_| |_|\___|_| |_|\__|___/ *
//*                       |___/                                             *
//===- include/pstore/support/array_elements.hpp --------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#ifndef PSTORE_SUPPORT_ARRAY_ELEMENTS_HPP
#define PSTORE_SUPPORT_ARRAY_ELEMENTS_HPP

#include <cstdlib>

namespace pstore {

    template <typename T, std::size_t Size>
    constexpr std::size_t array_elements (T (&)[Size]) noexcept {
        return Size;
    }

} // namespace pstore

#endif // PSTORE_SUPPORT_ARRAY_ELEMENTS_HPP
