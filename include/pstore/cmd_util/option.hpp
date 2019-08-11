//*              _   _              *
//*   ___  _ __ | |_(_) ___  _ __   *
//*  / _ \| '_ \| __| |/ _ \| '_ \  *
//* | (_) | |_) | |_| | (_) | | | | *
//*  \___/| .__/ \__|_|\___/|_| |_| *
//*       |_|                       *
//===- include/pstore/cmd_util/option.hpp ---------------------------------===//
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
#ifndef PSTORE_CMD_UTIL_OPTION_HPP
#define PSTORE_CMD_UTIL_OPTION_HPP

#include <cassert>
#include <list>

#include "parser.hpp"

namespace pstore {
    namespace cmd_util {
        namespace cl {

            enum class num_occurrences_flag {
                optional,     // Zero or One occurrence
                zero_or_more, // Zero or more occurrences allowed
                required,     // One occurrence required
                one_or_more,  // One or more occurrences required
            };
            class OptionCategory;

            //*           _   _           *
            //*  ___ _ __| |_(_)___ _ _   *
            //* / _ \ '_ \  _| / _ \ ' \  *
            //* \___/ .__/\__|_\___/_||_| *
            //*     |_|                   *
            class option {
            public:
                option (option const &) = delete;
                option (option &&) = delete;
                option & operator= (option const &) = delete;
                option & operator= (option &&) = delete;
                virtual ~option ();

                virtual void set_num_occurrences_flag (num_occurrences_flag n);
                virtual num_occurrences_flag get_num_occurrences_flag () const;

                virtual unsigned getNumOccurrences () const;
                bool is_satisfied () const;
                bool can_accept_another_occurrence () const;


                virtual void set_description (std::string const & d);
                std::string const & description () const;

                void set_category (OptionCategory const * cat) { category_ = cat; }

                virtual void set_positional ();
                virtual bool is_positional () const;

                virtual bool is_alias () const;
                virtual parser_base * get_parser () = 0;

                std::string const & name () const;
                void set_name (std::string const & name);

                virtual bool takes_argument () const = 0;
                virtual bool value (std::string const & v) = 0;
                virtual void add_occurrence ();

                using options_container = std::list<option *>;
                static options_container & all ();
                /// For unit testing.
                static options_container & reset_container ();

            protected:
                option ();
                explicit option (num_occurrences_flag occurrences);

            private:
                std::string name_;
                std::string description_;
                num_occurrences_flag occurrences_ = num_occurrences_flag::optional;
                bool positional_ = false;
                unsigned num_occurrences_ = 0U;
                OptionCategory const * category_ = nullptr;
            };

            template <typename Modifier>
            Modifier make_modifier (Modifier m) {
                return m;
            }

            class name;
            name make_modifier (char const * n);

            template <typename Option>
            inline void apply (Option &) {}
            template <typename Option, typename M0, typename... Mods>
            void apply (Option & opt, M0 const & m0, Mods const &... mods) {
                make_modifier (m0).apply (opt);
                apply (opt, mods...);
            }


            //*           _    *
            //*  ___ _ __| |_  *
            //* / _ \ '_ \  _| *
            //* \___/ .__/\__| *
            //*     |_|        *
            /// \tparam T The type produced by this option.
            /// \tparam ExternalStorage Included for compatibility with the LLVM command-line
            /// parser. Has no effect.
            /// \tparam Parser The parser which will convert from the user's string to type T.
            template <typename T, bool ExternalStorage = false, typename Parser = parser<T>>
            class opt final : public option {
            public:
                template <class... Mods>
                explicit opt (Mods const &... mods)
                        : option ()
                        , value_{} {
                    apply (*this, mods...);
                }
                opt (opt const &) = delete;
                opt (opt &&) = delete;
                opt & operator= (opt const &) = delete;
                opt & operator= (opt &&) = delete;

                template <typename U>
                void set_initial_value (U const & u) {
                    value_ = u;
                }

                explicit operator T () const { return get (); }
                T const & get () const noexcept { return value_; }

                bool empty () const { return value_.empty (); }
                bool value (std::string const & v) override;
                bool takes_argument () const override;
                parser_base * get_parser () override;

            private:
                T value_;
                Parser parser_;
            };

            template <typename T, bool ExternalStorage, typename Parser>
            bool opt<T, ExternalStorage, Parser>::takes_argument () const {
                return true;
            }
            template <typename T, bool ExternalStorage, typename Parser>
            bool opt<T, ExternalStorage, Parser>::value (std::string const & v) {
                if (auto m = parser_ (v)) {
                    value_ = m.value ();
                    return true;
                }
                return false;
            }
            template <typename T, bool ExternalStorage, typename Parser>
            parser_base * opt<T, ExternalStorage, Parser>::get_parser () {
                return &parser_;
            }


            //*           _     _              _  *
            //*  ___ _ __| |_  | |__  ___  ___| | *
            //* / _ \ '_ \  _| | '_ \/ _ \/ _ \ | *
            //* \___/ .__/\__| |_.__/\___/\___/_| *
            //*     |_|                           *
            template <>
            class opt<bool> final : public option {
            public:
                template <class... Mods>
                explicit opt (Mods const &... mods) {
                    apply (*this, mods...);
                }
                opt (opt const &) = delete;
                opt (opt &&) = delete;
                opt & operator= (opt const &) = delete;
                opt & operator= (opt &&) = delete;

                explicit operator bool () const noexcept { return get (); }
                bool get () const noexcept { return value_; }

                bool takes_argument () const override { return false; }
                bool value (std::string const & v) override;
                void add_occurrence () override;
                parser_base * get_parser () override;

                template <typename U>
                void set_initial_value (U const & u) {
                    value_ = u;
                }

            private:
                bool value_ = false;
            };

            //*  _ _    _    *
            //* | (_)__| |_  *
            //* | | (_-<  _| *
            //* |_|_/__/\__| *
            //*              *
            template <typename T, typename Parser = parser<T>>
            class list final : public option {
                using container = std::list<T>;

            public:
                using value_type = T;

                template <class... Mods>
                explicit list (Mods const &... mods)
                        : option (num_occurrences_flag::zero_or_more) {
                    apply (*this, mods...);
                }

                list (list const &) = delete;
                list (list &&) = delete;
                list & operator= (list const &) = delete;
                list & operator= (list &&) = delete;

                bool takes_argument () const override { return true; }
                bool value (std::string const & v) override;
                parser_base * get_parser () override;

                using iterator = typename container::const_iterator;
                using const_iterator = iterator;

                std::list<T> const & get () const noexcept { return values_; }

                const_iterator begin () const { return std::begin (values_); }
                const_iterator end () const { return std::end (values_); }
                std::size_t size () const { return values_.size (); }
                bool empty () const { return values_.empty (); }

            private:
                Parser parser_;
                std::list<T> values_;
            };

            template <typename T, typename Parser>
            bool list<T, Parser>::value (std::string const & v) {
                bool result = false;
                if (maybe<T> m = parser_ (v)) {
                    values_.push_back (m.value ());
                    result = true;
                }
                return result;
            }
            template <typename T, typename Parser>
            parser_base * list<T, Parser>::get_parser () {
                return nullptr;
            }

            //*       _ _          *
            //*  __ _| (_)__ _ ___ *
            //* / _` | | / _` (_-< *
            //* \__,_|_|_\__,_/__/ *
            //*                    *
            class alias final : public option {
            public:
                template <typename... Mods>
                explicit alias (Mods const &... mods) {
                    apply (*this, mods...);
                }

                alias (alias const &) = delete;
                alias (alias &&) = delete;
                alias & operator= (alias const &) = delete;
                alias & operator= (alias &&) = delete;

                void set_num_occurrences_flag (num_occurrences_flag n) override;
                num_occurrences_flag get_num_occurrences_flag () const override;
                void set_positional () override;
                bool is_positional () const override;
                bool is_alias () const override;
                unsigned getNumOccurrences () const override;
                parser_base * get_parser () override;
                bool takes_argument () const override;
                bool value (std::string const & v) override;

                void set_original (option * o);

            private:
                option * original_ = nullptr;
            };

        } // end namespace cl
    }     // end namespace cmd_util
} // end namespace pstore

#endif // PSTORE_CMD_UTIL_OPTION_H
