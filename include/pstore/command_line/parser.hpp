//*                                  *
//*  _ __   __ _ _ __ ___  ___ _ __  *
//* | '_ \ / _` | '__/ __|/ _ \ '__| *
//* | |_) | (_| | |  \__ \  __/ |    *
//* | .__/ \__,_|_|  |___/\___|_|    *
//* |_|                              *
//===- include/pstore/command_line/parser.hpp -----------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_PARSER_HPP
#define PSTORE_COMMAND_LINE_PARSER_HPP

#include <algorithm>
#include <cerrno>
#include <cstdlib>

#include "pstore/support/gsl.hpp"
#include "pstore/support/maybe.hpp"

namespace pstore {
    namespace command_line {

        // This represents a single enum value, using "int" as the underlying type.
        struct literal {
            literal () = default;
            literal (std::string const & n, int const v, std::string const & d)
                    : name{n}
                    , value{v}
                    , description{d} {}
            literal (std::string const & n, int const v)
                    : literal (n, v, n) {}
            std::string name;
            int value = 0;
            std::string description;
        };


        //*                               _                   *
        //*  _ __  __ _ _ _ ___ ___ _ _  | |__  __ _ ___ ___  *
        //* | '_ \/ _` | '_(_-</ -_) '_| | '_ \/ _` (_-</ -_) *
        //* | .__/\__,_|_| /__/\___|_|   |_.__/\__,_/__/\___| *
        //* |_|                                               *
        class parser_base {
            using container = std::vector<literal>;

        public:
            virtual ~parser_base () noexcept;
            void add_literal_option (std::string const & name, int value,
                                     std::string const & description);

            using iterator = container::iterator;
            using const_iterator = container::const_iterator;

            const_iterator begin () const { return std::begin (literals_); }
            const_iterator end () const { return std::end (literals_); }

        private:
            container literals_;
        };


        //*                              *
        //*  _ __  __ _ _ _ ___ ___ _ _  *
        //* | '_ \/ _` | '_(_-</ -_) '_| *
        //* | .__/\__,_|_| /__/\___|_|   *
        //* |_|                          *
        template <typename T, typename = void>
        class parser : public parser_base {
        public:
            maybe<T> operator() (std::string const & v) const;
        };

        template <typename T>
        class parser<T, typename std::enable_if<std::is_enum<T>::value>::type>
                : public parser_base {
        public:
            maybe<T> operator() (std::string const & v) const {
                auto begin = this->begin ();
                auto end = this->end ();
                auto it =
                    std::find_if (begin, end, [&v] (literal const & lit) { return v == lit.name; });
                if (it == end) {
                    return nothing<T> ();
                }
                return just (T (it->value));
            }
        };


        template <typename T>
        class parser<T, typename std::enable_if<std::is_integral<T>::value>::type>
                : public parser_base {
        public:
            maybe<T> operator() (std::string const & v) const;
        };

        template <typename T>
        maybe<T> parser<T, typename std::enable_if<std::is_integral<T>::value>::type>::operator() (
            std::string const & v) const {
            PSTORE_ASSERT (std::distance (this->begin (), this->end ()) == 0 &&
                           "Don't specify literal values for an integral option!");
            if (v.length () == 0) {
                return nothing<T> ();
            }
            gsl::zstring str_end = nullptr;
            gsl::czstring const str = v.c_str ();
            errno = 0;
            long const res = std::strtol (str, &str_end, 10);
            if (str_end != str + v.length () || errno != 0 ||
                res > std::numeric_limits<int>::max () || res < std::numeric_limits<int>::min ()) {
                return nothing<T> ();
            }
            return just (static_cast<T> (res));
        }


        //*                                  _       _            *
        //*  _ __  __ _ _ _ ___ ___ _ _   __| |_ _ _(_)_ _  __ _  *
        //* | '_ \/ _` | '_(_-</ -_) '_| (_-<  _| '_| | ' \/ _` | *
        //* | .__/\__,_|_| /__/\___|_|   /__/\__|_| |_|_||_\__, | *
        //* |_|                                            |___/  *
        template <>
        class parser<std::string> : public parser_base {
        public:
            ~parser () noexcept override;
            maybe<std::string> operator() (std::string const & v) const;
        };

    } // end namespace command_line
} // end namespace pstore

#endif // PSTORE_COMMAND_LINE_PARSER_HPP
