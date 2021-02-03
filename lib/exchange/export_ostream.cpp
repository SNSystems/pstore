//*                             _               _                             *
//*   _____  ___ __   ___  _ __| |_    ___  ___| |_ _ __ ___  __ _ _ __ ___   *
//*  / _ \ \/ / '_ \ / _ \| '__| __|  / _ \/ __| __| '__/ _ \/ _` | '_ ` _ \  *
//* |  __/>  <| |_) | (_) | |  | |_  | (_) \__ \ |_| | |  __/ (_| | | | | | | *
//*  \___/_/\_\ .__/ \___/|_|   \__|  \___/|___/\__|_|  \___|\__,_|_| |_| |_| *
//*           |_|                                                             *
//===- lib/exchange/export_ostream.cpp ------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/export_ostream.hpp"

#include <cinttypes>

namespace pstore {
    namespace exchange {
        namespace export_ns {

            // write
            // ~~~~~
            ostream & ostream::write (char const c) {
                std::fputc (c, os_);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
                return *this;
            }
            ostream & ostream::write (std::uint16_t const v) {
                std::fprintf (os_, "%" PRIu16, v);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
                return *this;
            }
            ostream & ostream::write (std::uint32_t const v) {
                std::fprintf (os_, "%" PRIu32, v);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
                return *this;
            }
            ostream & ostream::write (std::int64_t const v) {
                std::fprintf (os_, "%" PRId64, v);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
                return *this;
            }
            ostream & ostream::write (std::uint64_t const v) {
                std::fprintf (os_, "%" PRIu64, v);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
                return *this;
            }
            ostream & ostream::write (gsl::czstring const str) {
                std::fputs (str, os_);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
                return *this;
            }
            ostream & ostream::write (std::string const & str) {
                std::fputs (str.c_str (), os_);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
                return *this;
            }

            // flush
            // ~~~~~
            void ostream::flush () {
                std::fflush (os_);
                if (ferror (os_)) {
                    raise (error_code::write_failed);
                }
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore
