//*                                  *
//*  _ __   __ _ _ __ ___  ___ _ __  *
//* | '_ \ / _` | '__/ __|/ _ \ '__| *
//* | |_) | (_| | |  \__ \  __/ |    *
//* | .__/ \__,_|_|  |___/\___|_|    *
//* |_|                              *
//===- include/pstore/cmd_util/cl/parser.hpp ------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_CMD_UTIL_CL_PARSER_HPP
#define PSTORE_CMD_UTIL_CL_PARSER_HPP

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <string>
#include <vector>

#include "pstore/support/maybe.hpp"

namespace pstore {
    namespace cmd_util {
        namespace cl {

            //*                               _                   *
            //*  _ __  __ _ _ _ ___ ___ _ _  | |__  __ _ ___ ___  *
            //* | '_ \/ _` | '_(_-</ -_) '_| | '_ \/ _` (_-</ -_) *
            //* | .__/\__,_|_| /__/\___|_|   |_.__/\__,_/__/\___| *
            //* |_|                                               *
            class parser_base {
            public:
                virtual ~parser_base ();
                void add_literal_option (std::string const & name, int value,
                                         std::string const & description);

            protected:
                struct literal {
                    literal (std::string const & name_, int value_,
                             std::string const & description_)
                            : name{name_}
                            , value{value_}
                            , description{description_} {}
                    std::string name;
                    int value;
                    std::string const & description;
                };

                using container = std::vector<literal>;
                using iterator = container::iterator;
                using const_iterator = container::const_iterator;

                const_iterator begin () const { return std::begin (literals_); }
                const_iterator end () const { return std::end (literals_); }

            private:
                std::vector<literal> literals_;
            };


            //*                              *
            //*  _ __  __ _ _ _ ___ ___ _ _  *
            //* | '_ \/ _` | '_(_-</ -_) '_| *
            //* | .__/\__,_|_| /__/\___|_|   *
            //* |_|                          *
            template <typename T>
            class parser : public parser_base {
            public:
                maybe<T> operator() (std::string const & v) const;
            };

            template <typename T>
            maybe<T> parser<T>::operator() (std::string const & v) const {
                auto begin = this->begin ();
                auto end = this->end ();
                if (std::distance (begin, end) == 0) {
                    if (v.length () == 0) {
                        return nothing<T> ();
                    }
                    char * str_end = nullptr;
                    char const * str = v.c_str ();
                    errno = 0;
                    long const res = std::strtol (str, &str_end, 10);
                    if (str_end != str + v.length () || errno != 0 ||
                        res > std::numeric_limits<int>::max () ||
                        res < std::numeric_limits<int>::min ()) {
                        return nothing<T> ();
                    }
                    return just (static_cast<T> (res));
                } else {
                    auto it = std::find_if (begin, end,
                                            [&v](literal const & lit) { return v == lit.name; });
                    if (it == end) {
                        return nothing<T> ();
                    }
                    return just (T (it->value));
                }
            }

            //*                                  _       _            *
            //*  _ __  __ _ _ _ ___ ___ _ _   __| |_ _ _(_)_ _  __ _  *
            //* | '_ \/ _` | '_(_-</ -_) '_| (_-<  _| '_| | ' \/ _` | *
            //* | .__/\__,_|_| /__/\___|_|   /__/\__|_| |_|_||_\__, | *
            //* |_|                                            |___/  *
            template <>
            class parser<std::string> : public parser_base {
            public:
                ~parser () override;
                maybe<std::string> operator() (std::string const & v) const;
            };
        } // namespace cl
    }     // namespace cmd_util
} // namespace pstore

#endif // PSTORE_CMD_UTIL_CL_PARSER_HPP
