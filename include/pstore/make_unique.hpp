//*                  _                      _                   *
//*  _ __ ___   __ _| | _____   _   _ _ __ (_) __ _ _   _  ___  *
//* | '_ ` _ \ / _` | |/ / _ \ | | | | '_ \| |/ _` | | | |/ _ \ *
//* | | | | | | (_| |   <  __/ | |_| | | | | | (_| | |_| |  __/ *
//* |_| |_| |_|\__,_|_|\_\___|  \__,_|_| |_|_|\__, |\__,_|\___| *
//*                                              |_|            *
//===- include/pstore/make_unique.hpp -------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc. 
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
/// \file make_unique.hpp
/// \brief An implementation of std::make_unique<>() for platforms that do not provide it natively.

#ifndef PSTORE_MAKE_UNIQUE_HPP
#define PSTORE_MAKE_UNIQUE_HPP (1)

// TODO: determine make_unique support at configure time.
#if __cplusplus <= 201103L && !defined(_WIN32)

#include <memory>
#include <type_traits>
#include <utility>

namespace std {
    template <typename T, typename... Args>
    std::unique_ptr<T> make_unique_helper (std::false_type, Args &&... args) {
        return std::unique_ptr<T> (new T (std::forward<Args> (args)...));
    }

    template <typename T, typename = typename std::enable_if<std::extent<T>::value == 0>::type>
    std::unique_ptr<T> make_unique_helper (std::true_type, std::size_t size) {
        using U = typename std::remove_extent<T>::type;
        return std::unique_ptr<T> (new U[size]);
    }

    template <typename T, typename... Args>
    std::unique_ptr<T> make_unique (Args &&... args) {
        return make_unique_helper<T> (std::is_array<T> (), std::forward<Args> (args)...);
    }
} // namespace std
#endif

#endif // PSTORE_MAKE_UNIQUE_HPP
// eof: include/pstore/make_unique.hpp
