//*  _     _   _               _       _        *
//* | |__ | |_| |_ _ __     __| | __ _| |_ ___  *
//* | '_ \| __| __| '_ \   / _` |/ _` | __/ _ \ *
//* | | | | |_| |_| |_) | | (_| | (_| | ||  __/ *
//* |_| |_|\__|\__| .__/   \__,_|\__,_|\__\___| *
//*               |_|                           *
//===- lib/http/http_date.cpp ---------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/http/http_date.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>

#include "pstore/os/time.hpp"

namespace {

    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
    constexpr std::size_t as_index (T const v, std::size_t const size) noexcept {
        return std::min (static_cast<std::size_t> (std::max (v, T{0})), size - std::size_t{1});
    }

} // end anonymous namespace

namespace pstore {
    namespace http {

        // Produce a date in http-date format (https://tools.ietf.org/html/rfc7231#page-65)
        std::string http_date (time_t const time) {
            std::tm const t = gm_time (time);

            // day-name = %x4D.6F.6E ; "Mon", case-sensitive
            //          / %x54.75.65 ; "Tue", case-sensitive
            //          / %x57.65.64 ; "Wed", case-sensitive
            //          / %x54.68.75 ; "Thu", case-sensitive
            //          / %x46.72.69 ; "Fri", case-sensitive
            //          / %x53.61.74 ; "Sat", case-sensitive
            //          / %x53.75.6E ; "Sun", case-sensitive
            static constexpr std::array<char const *, 7> days{
                {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}};
            auto const day_name = days[as_index (t.tm_wday, days.size ())];

            // month = %x4A.61.6E ; "Jan", case-sensitive
            //       / %x46.65.62 ; "Feb", case-sensitive
            //       / %x4D.61.72 ; "Mar", case-sensitive
            //       / %x41.70.72 ; "Apr", case-sensitive
            //       / %x4D.61.79 ; "May", case-sensitive
            //       / %x4A.75.6E ; "Jun", case-sensitive
            //       / %x4A.75.6C ; "Jul", case-sensitive
            //       / %x41.75.67 ; "Aug", case-sensitive
            //       / %x53.65.70 ; "Sep", case-sensitive
            //       / %x4F.63.74 ; "Oct", case-sensitive
            //       / %x4E.6F.76 ; "Nov", case-sensitive
            //       / %x44.65.63 ; "Dec", case-sensitive
            static constexpr std::array<char const *, 12> months{{"Jan", "Feb", "Mar", "Apr", "May",
                                                                  "Jun", "Jul", "Aug", "Sep", "Oct",
                                                                  "Nov", "Dec"}};
            auto const month = months[as_index (t.tm_mon, months.size ())];

            // hour         = 2DIGIT
            // minute       = 2DIGIT
            // second       = 2DIGIT
            // time-of-day  = hour ":" minute ":" second
            //              ; 00:00:00 - 23:59:60 (leap second)
            // year         = 4DIGIT
            // day          = 2DIGIT
            // date1        = day SP month SP year
            //              ; e.g., 02 Jun 1982
            // IMF-fixdate  = day-name "," SP date1 SP time-of-day SP GMT
            std::ostringstream fixdate;
            fixdate << std::setfill ('0') << day_name << ", " << std::setw (2) << t.tm_mday << ' '
                    << month << ' ' << std::setw (4) << t.tm_year + 1900 << ' ' << std::setw (2)
                    << t.tm_hour << ':' << std::setw (2) << t.tm_min << ':' << std::setw (2)
                    << t.tm_sec << " GMT";
            return fixdate.str ();
        }

        std::string http_date (std::chrono::system_clock::time_point const time) {
            return http_date (std::chrono::system_clock::to_time_t (time));
        }

    } // end namespace http
} // end namespace pstore
