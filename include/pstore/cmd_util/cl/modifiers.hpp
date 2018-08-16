//*                      _ _  __ _                *
//*  _ __ ___   ___   __| (_)/ _(_) ___ _ __ ___  *
//* | '_ ` _ \ / _ \ / _` | | |_| |/ _ \ '__/ __| *
//* | | | | | | (_) | (_| | |  _| |  __/ |  \__ \ *
//* |_| |_| |_|\___/ \__,_|_|_| |_|\___|_|  |___/ *
//*                                               *
//===- include/pstore/cmd_util/cl/modifiers.hpp ---------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_CMD_UTIL_CL_MODIFIERS_HPP
#define PSTORE_CMD_UTIL_CL_MODIFIERS_HPP

#include <cassert>
#include <string>
#include <vector>

#include "pstore/cmd_util/cl/category.hpp"
#include "pstore/cmd_util/cl/option.hpp"
#include "pstore/cmd_util/cl/parser.hpp"

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

            // This represents a single enum value, using "int" as the underlying type.
            struct OptionEnumValue {
                std::string name;
                int value;
                std::string description;
            };

            // values - For custom data types, allow specifying a group of values together
            // as the values that go into the mapping that the option handler uses.
            namespace details {
                class values {
                public:
                    values (std::initializer_list<OptionEnumValue> options);

                    template <class Opt>
                    void apply (Opt & o) const {
                        if (parser_base * const p = o.get_parser ()) {
                            for (auto const & v : values_) {
                                p->add_literal_option (v.name, v.value, v.description);
                            }
                        }
                    }

                private:
                    std::vector<OptionEnumValue> values_;
                };
            } // namespace details

            /// Helper to build a ValuesClass by forwarding a variable number of arguments
            /// as an initializer list to the ValuesClass constructor.
            template <typename... OptsTy>
            details::values values (OptsTy... options) {
                return {options...};
            }

            class name {
            public:
                name (std::string const & name);

                template <typename Opt>
                void apply (Opt & o) const {
                    o.set_name (name_);
                }

            private:
                std::string const name_;
            };

            inline name make_modifier (char const * n) { return name (n); }


            //*     _             *
            //*  __| |___ ___ __  *
            //* / _` / -_|_-</ _| *
            //* \__,_\___/__/\__| *
            //*                   *
            /// A modifier to set the description shown in the -help output...
            class desc {
            public:
                explicit desc (std::string const & str);

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
                    initializer (T const & t)
                            : init_{t} {}
                    template <class Opt>
                    void apply (Opt & o) const {
                        o.set_initial_value (init_);
                    }

                private:
                    T const & init_;
                };
            } // namespace details

            template <typename T>
            details::initializer<T> init (T const & t) {
                return {t};
            }

            //*                                              *
            //*  ___  __ __ _  _ _ _ _ _ ___ _ _  __ ___ ___ *
            //* / _ \/ _/ _| || | '_| '_/ -_) ' \/ _/ -_|_-< *
            //* \___/\__\__|\_,_|_| |_| \___|_||_\__\___/__/ *
            //*                                              *
            namespace details {
                struct positional {
                    // The need for this constructor was removed by CWG defect 253 but Clang (prior
                    // to 3.9.0) and GCC (before 4.6.4) require its presence.
                    positional () noexcept {}

                    template <typename Opt>
                    void apply (Opt & o) const {
                        o.set_positional ();
                    }
                };

                struct required {
                    // The need for this constructor was removed by CWG defect 253 but Clang (prior
                    // to 3.9.0) and GCC (before 4.6.4) require its presence.
                    required () noexcept {}

                    template <typename Opt>
                    void apply (Opt & o) const {
                        o.set_num_occurrences_flag (num_occurrences_flag::required);
                    }
                };

                struct optional {
                    // The need for this constructor was removed by CWG defect 253 but Clang (prior
                    // to 3.9.0) and GCC (before 4.6.4) require its presence.
                    optional () noexcept {}

                    template <typename Opt>
                    void apply (Opt & o) const {
                        o.set_num_occurrences_flag (num_occurrences_flag::optional);
                    }
                };

                struct one_or_more {
                    // The need for this constructor was removed by CWG defect 253 but Clang (prior
                    // to 3.9.0) and GCC (before 4.6.4) require its presence.
                    one_or_more () noexcept {}

                    template <typename Opt>
                    void apply (Opt & o) const {
                        bool is_optional =
                            o.get_num_occurrences_flag () == num_occurrences_flag::optional;
                        o.set_num_occurrences_flag (is_optional
                                                        ? num_occurrences_flag::zero_or_more
                                                        : num_occurrences_flag::one_or_more);
                    }
                };
            } // namespace details

            extern details::one_or_more const OneOrMore;
            extern details::optional const Optional;
            extern details::positional const Positional;
            extern details::required const Required;

            namespace details {
                class category {
                public:
                    category (OptionCategory const & cat)
                            : cat_{cat} {}

                    template <typename Opt>
                    void apply (Opt & o) const {
                        o.set_category (&cat_);
                    }

                private:
                    OptionCategory const & cat_;
                };
            } // namespace details

            inline details::category cat (OptionCategory const & c) { return {c}; }

        } // namespace cl
    }     // namespace cmd_util
} // namespace pstore

#endif // PSTORE_CMD_UTIL_CL_MODIFIERS_HPP
