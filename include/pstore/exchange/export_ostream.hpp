//*                             _               _                             *
//*   _____  ___ __   ___  _ __| |_    ___  ___| |_ _ __ ___  __ _ _ __ ___   *
//*  / _ \ \/ / '_ \ / _ \| '__| __|  / _ \/ __| __| '__/ _ \/ _` | '_ ` _ \  *
//* |  __/>  <| |_) | (_) | |  | |_  | (_) \__ \ |_| | |  __/ (_| | | | | | | *
//*  \___/_/\_\ .__/ \___/|_|   \__|  \___/|___/\__|_|  \___|\__,_|_| |_| |_| *
//*           |_|                                                             *
//===- include/pstore/exchange/export_ostream.hpp -------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_EXPORT_OSTREAM_HPP
#define PSTORE_EXCHANGE_EXPORT_OSTREAM_HPP

#include <cstdio>

#include "pstore/core/indirect_string.hpp"
#include "pstore/support/unsigned_cast.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            class ostream {
            public:
                explicit ostream (FILE * const os)
                        : os_ (os) {}
                ostream (ostream const &) = delete;
                ostream (ostream &&) = delete;

                ~ostream () noexcept = default;

                ostream & operator= (ostream const &) = delete;
                ostream & operator= (ostream &&) = delete;

                ostream & write (char c);
                ostream & write (std::uint16_t v);
                ostream & write (std::uint32_t v);
                ostream & write (std::uint64_t v);
                ostream & write (gsl::czstring str);
                ostream & write (std::string const & str);

                template <typename T, typename = typename std::enable_if<
                                          std::is_trivially_copyable<T>::value>::type>
                ostream & write (T const * s, std::streamsize const length) {
                    std::fwrite (s, sizeof (T),
                                 unsigned_cast<decltype (length), std::size_t> (length), os_);
                    if (ferror (os_)) {
                        raise (error_code::write_failed);
                    }
                    return *this;
                }

                void flush ();

            private:
                FILE * os_;
            };

            inline ostream & operator<< (ostream & os, char const c) { return os.write (c); }
            inline ostream & operator<< (ostream & os, std::uint16_t const v) {
                return os.write (v);
            }
            inline ostream & operator<< (ostream & os, std::uint32_t const v) {
                return os.write (v);
            }
            inline ostream & operator<< (ostream & os, std::uint64_t const v) {
                return os.write (v);
            }
            inline ostream & operator<< (ostream & os, gsl::czstring const str) {
                return os.write (str);
            }
            inline ostream & operator<< (ostream & os, std::string const & str) {
                return os.write (str);
            }
            inline ostream & operator<< (ostream & os, indirect_string const & ind_str) {
                shared_sstring_view owner;
                return os << ind_str.as_string_view (&owner);
            }


            class ostream_inserter : public std::iterator<std::output_iterator_tag, char> {
            public:
                explicit ostream_inserter (ostream & os)
                        : os_{os} {}

                ostream_inserter & operator= (char const c) {
                    os_ << c;
                    return *this;
                }
                ostream_inserter & operator* () noexcept { return *this; }
                ostream_inserter & operator++ () noexcept { return *this; }
                ostream_inserter operator++ (int) noexcept { return *this; }

            private:
                ostream & os_;
            };

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_OSTREAM_HPP
