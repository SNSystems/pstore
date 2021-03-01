//===- include/pstore/http/http_date.hpp ------------------*- mode: C++ -*-===//
//*  _     _   _               _       _        *
//* | |__ | |_| |_ _ __     __| | __ _| |_ ___  *
//* | '_ \| __| __| '_ \   / _` |/ _` | __/ _ \ *
//* | | | | |_| |_| |_) | | (_| | (_| | ||  __/ *
//* |_| |_|\__|\__| .__/   \__,_|\__,_|\__\___| *
//*               |_|                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file http_date.hpp
/// \brief Functions to return a dated formatted for HTTP.
#ifndef PSTORE_HTTP_HTTP_DATE_HPP
#define PSTORE_HTTP_HTTP_DATE_HPP

#include <chrono>
#include <ctime>
#include <string>

namespace pstore {
    namespace http {

        std::string http_date (std::chrono::system_clock::time_point time);
        std::string http_date (std::time_t time);

    } // end namespace http
} // end namespace pstore

#endif // PSTORE_HTTP_HTTP_DATE_HPP
