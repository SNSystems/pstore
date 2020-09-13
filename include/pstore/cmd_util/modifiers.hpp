//*                      _ _  __ _                *
//*  _ __ ___   ___   __| (_)/ _(_) ___ _ __ ___  *
//* | '_ ` _ \ / _ \ / _` | | |_| |/ _ \ '__/ __| *
//* | | | | | | (_) | (_| | |  _| |  __/ |  \__ \ *
//* |_| |_| |_|\___/ \__,_|_|_| |_|\___|_|  |___/ *
//*                                               *
//===- include/pstore/cmd_util/modifiers.hpp ------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_CMD_UTIL_MODIFIERS_HPP
#define PSTORE_CMD_UTIL_MODIFIERS_HPP

#include <cassert>
#include <string>
#include <utility>

#include "pstore/adt/small_vector.hpp"
#include "pstore/cmd_util/category.hpp"
#include "pstore/cmd_util/option.hpp"
#include "pstore/cmd_util/parser.hpp"

namespace pstore {
    namespace cmd_util {
        namespace cl {

            //*           _              *
            //* __ ____ _| |_  _ ___ ___ *
            //* \ V / _` | | || / -_|_-< *
            //*  \_/\__,_|_|\_,_\___/__/ *
            //*                          *
            //===----------------------------------------------------------------------===//
            // Enum valued command line option
            //

            // values - For custom data types, allow specifying a group of values together
            // as the values that go into the mapping that the option handler uses.
            namespace details {

                class values {
                public:
                    explicit values (std::initializer_list<literal> options);

                    template <class Opt>
                    void apply (Opt & o) const {
                        if (parser_base * const p = o.get_parser ()) {
                            for (auto const & v : values_) {
                                p->add_literal_option (v.name, v.value, v.description);
                            }
                        }
                    }

                private:
                    small_vector<literal, 3> values_;
                };

            } // end namespace details

            /// Helper to build a details::values by forwarding a variable number of arguments
            /// as an initializer list to the details::values constructor.
            template <typename... OptsTy>
            details::values values (OptsTy &&... options) {
                return details::values{std::forward<OptsTy> (options)...};
            }

            inline details::values values (std::initializer_list<literal> options) {
                return details::values (options);
            }

            class name {
            public:
                explicit name (std::string name);

                template <typename Opt>
                void apply (Opt & o) const {
                    o.set_name (name_);
                }

            private:
                std::string const name_;
            };

            inline name make_modifier (gsl::czstring const n) { return name (n); }
            inline name make_modifier (std::string const & n) { return name (n); }


            /// A modifier to set the usage information shown in the -help output.
            /// Only applicable to positional arguments.
            class usage {
            public:
                explicit usage (std::string str);

                template <typename Opt>
                void apply (Opt & o) const {
                    o.set_usage (desc_);
                }

            private:
                std::string const desc_;
            };

            //*     _             *
            //*  __| |___ ___ __  *
            //* / _` / -_|_-</ _| *
            //* \__,_\___/__/\__| *
            //*                   *
            /// A modifier to set the description shown in the -help output...
            class desc {
            public:
                explicit desc (std::string str);

                template <typename Opt>
                void apply (Opt & o) const {
                    o.set_description (desc_);
                }

            private:
                std::string const desc_;
            };

            //*       _ _                   _    *
            //*  __ _| (_)__ _ ___ ___ _ __| |_  *
            //* / _` | | / _` (_-</ _ \ '_ \  _| *
            //* \__,_|_|_\__,_/__/\___/ .__/\__| *
            //*                       |_|        *
            class aliasopt {
            public:
                explicit aliasopt (option & o);
                void apply (alias & o) const;

            private:
                option & original_;
            };

            //*  _      _ _    *
            //* (_)_ _ (_) |_  *
            //* | | ' \| |  _| *
            //* |_|_||_|_|\__| *
            //*                *
            namespace details {

                template <typename T>
                class initializer {
                public:
                    template <typename U>
                    explicit initializer (U && t)
                            : init_{std::forward<U> (t)} {}
                    template <class Opt>
                    void apply (Opt & o) const {
                        o.set_initial_value (init_);
                    }

                private:
                    T const & init_;
                };

            } // end namespace details

            template <typename T>
            details::initializer<T> init (T && t) {
                return details::initializer<T>{std::forward<T> (t)};
            }

            namespace details {

                struct comma_separated {
                    // The need for this constructor was removed by CWG defect 253 but Clang (prior
                    // to 3.9.0) and GCC (before 4.6.4) require its presence.
                    constexpr comma_separated () noexcept {} // NOLINT

                    template <typename Opt>
                    void apply (Opt & o) const {
                        o.set_comma_separated ();
                    }
                };

            } // end namespace details

            /// When this modifier is added to a list option, it will consider each of the argument
            /// strings to be a sequence of one or more comma-separated values. These are broken
            /// apart before being passed to the argument parser. The modifier has no effect on
            /// other option types.
            ///
            /// For example, a list option named "opt" with comma-separated
            /// enabled will consider command-lines such as "--opt a,b,c", "--opt a,b --opt c", and
            /// "--opt a --opt b --opt c" to be equivalent. Without the option "--opt a,b" is has a
            /// single value "a,b".
            extern details::comma_separated const comma_separated;

            //*                                              *
            //*  ___  __ __ _  _ _ _ _ _ ___ _ _  __ ___ ___ *
            //* / _ \/ _/ _| || | '_| '_/ -_) ' \/ _/ -_|_-< *
            //* \___/\__\__|\_,_|_| |_| \___|_||_\__\___/__/ *
            //*                                              *
            namespace details {

                struct positional {
                    // The need for this constructor was removed by CWG defect 253 but Clang (prior
                    // to 3.9.0) and GCC (before 4.6.4) require its presence.
                    constexpr positional () noexcept {} // NOLINT

                    template <typename Opt>
                    void apply (Opt & o) const {
                        o.set_positional ();
                    }
                };

                struct required {
                    // The need for this constructor was removed by CWG defect 253 but Clang (prior
                    // to 3.9.0) and GCC (before 4.6.4) require its presence.
                    constexpr required () noexcept {} // NOLINT

                    template <typename Opt>
                    void apply (Opt & o) const {
                        o.set_num_occurrences_flag (num_occurrences_flag::required);
                    }
                };

                struct optional {
                    // The need for this constructor was removed by CWG defect 253 but Clang (prior
                    // to 3.9.0) and GCC (before 4.6.4) require its presence.
                    constexpr optional () noexcept {} // NOLINT

                    template <typename Opt>
                    void apply (Opt & o) const {
                        o.set_num_occurrences_flag (num_occurrences_flag::optional);
                    }
                };

                struct one_or_more {
                    // The need for this constructor was removed by CWG defect 253 but Clang (prior
                    // to 3.9.0) and GCC (before 4.6.4) require its presence.
                    constexpr one_or_more () noexcept {} // NOLINT

                    template <typename Opt>
                    void apply (Opt & o) const {
                        bool const is_optional =
                            o.get_num_occurrences_flag () == num_occurrences_flag::optional;
                        o.set_num_occurrences_flag (is_optional
                                                        ? num_occurrences_flag::zero_or_more
                                                        : num_occurrences_flag::one_or_more);
                    }
                };

            } // end namespace details

            extern details::one_or_more const one_or_more;
            extern details::optional const optional;
            extern details::positional const positional;
            extern details::required const required;

            namespace details {

                class category {
                public:
                    explicit constexpr category (option_category const & cat) noexcept
                            : cat_{cat} {}

                    template <typename Opt>
                    void apply (Opt & o) const {
                        o.set_category (&cat_);
                    }

                private:
                    option_category const & cat_;
                };

            } // end namespace details

            inline details::category cat (option_category const & c) {
                return details::category{c};
            }

        } // namespace cl
    }     // namespace cmd_util
} // namespace pstore

#endif // PSTORE_CMD_UTIL_MODIFIERS_HPP
