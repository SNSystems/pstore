//===- include/pstore/command_line/option.hpp -------------*- mode: C++ -*-===//
//*              _   _              *
//*   ___  _ __ | |_(_) ___  _ __   *
//*  / _ \| '_ \| __| |/ _ \| '_ \  *
//* | (_) | |_) | |_| | (_) | | | | *
//*  \___/| .__/ \__|_|\___/|_| |_| *
//*       |_|                       *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_OPTION_HPP
#define PSTORE_COMMAND_LINE_OPTION_HPP

#include "pstore/command_line/csv.hpp"
#include "pstore/command_line/parser.hpp"

namespace pstore {
    namespace command_line {

        template <typename T, typename = void>
        struct type_description {};
        template <>
        struct type_description<std::string> {
            static gsl::czstring value;
        };
        template <>
        struct type_description<int> {
            static gsl::czstring value;
        };
        template <>
        struct type_description<long> {
            static gsl::czstring value;
        };
        template <>
        struct type_description<long long> {
            static gsl::czstring value;
        };
        template <>
        struct type_description<unsigned short> {
            static gsl::czstring value;
        };
        template <>
        struct type_description<unsigned int> {
            static gsl::czstring value;
        };
        template <>
        struct type_description<unsigned long> {
            static gsl::czstring value;
        };
        template <>
        struct type_description<unsigned long long> {
            static gsl::czstring value;
        };
        template <typename T>
        struct type_description<T, typename std::enable_if<std::is_enum<T>::value>::type> {
            static gsl::czstring value;
        };
        template <typename T>
        gsl::czstring
            type_description<T, typename std::enable_if<std::is_enum<T>::value>::type>::value =
                "enum";

        enum class num_occurrences_flag {
            optional,     // Zero or One occurrence
            zero_or_more, // Zero or more occurrences allowed
            required,     // One occurrence required
            one_or_more,  // One or more occurrences required
        };
        class option_category;
        class alias;

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

            virtual unsigned get_num_occurrences () const;
            bool is_satisfied () const;
            bool can_accept_another_occurrence () const;


            virtual void set_description (std::string const & d);
            std::string const & description () const noexcept;

            virtual void set_usage (std::string const & d);
            std::string const & usage () const noexcept;

            void set_comma_separated () noexcept { comma_separated_ = true; }
            bool allow_comma_separated () const noexcept { return comma_separated_; }

            void set_category (option_category const * const cat) { category_ = cat; }
            virtual option_category const * category () const noexcept { return category_; }

            virtual void set_positional ();
            virtual bool is_positional () const;

            virtual bool is_alias () const;
            virtual alias * as_alias ();
            virtual alias const * as_alias () const;

            virtual parser_base * get_parser () = 0;

            std::string const & name () const;
            void set_name (std::string const & name);

            virtual bool takes_argument () const = 0;
            virtual bool value (std::string const & v) = 0;
            virtual bool add_occurrence ();

            using options_container = std::list<option *>;
            static options_container & all ();
            /// For unit testing.
            static options_container & reset_container ();

            virtual gsl::czstring arg_description () const noexcept;

        protected:
            option ();
            explicit option (num_occurrences_flag occurrences);

        private:
            std::string name_;
            std::string usage_;
            std::string description_;
            num_occurrences_flag occurrences_ = num_occurrences_flag::optional;
            bool positional_ = false;
            bool comma_separated_ = false;
            unsigned num_occurrences_ = 0U;
            option_category const * category_ = nullptr;
        };

        template <typename Modifier>
        Modifier && make_modifier (Modifier && m) {
            return std::forward<Modifier> (m);
        }

        class name;
        name make_modifier (gsl::czstring n);
        name make_modifier (std::string const & n);

        template <typename Option>
        void apply_to_option (Option &&) {}

        template <typename Option, typename M0, typename... Mods>
        void apply_to_option (Option && opt, M0 && m0, Mods &&... mods) {
            make_modifier (std::forward<M0> (m0)).apply (std::forward<Option> (opt));
            apply_to_option (std::forward<Option> (opt), std::forward<Mods> (mods)...);
        }


        //*           _    *
        //*  ___ _ __| |_  *
        //* / _ \ '_ \  _| *
        //* \___/ .__/\__| *
        //*     |_|        *
        /// \tparam T The type produced by this option.
        /// \tparam Parser The parser which will convert from the user's string to type T.
        template <typename T, typename Parser = parser<T>>
        class opt final : public option {
        public:
            template <class... Mods>
            explicit opt (Mods const &... mods)
                    : option () {
                apply_to_option (*this, mods...);
            }
            opt (opt const &) = delete;
            opt (opt &&) = delete;
            ~opt () noexcept override = default;

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

            gsl::czstring arg_description () const noexcept override {
                return type_description<T>::value;
            }

        private:
            T value_{};
            Parser parser_;
        };

        template <typename T, typename Parser>
        bool opt<T, Parser>::takes_argument () const {
            return true;
        }
        template <typename T, typename Parser>
        bool opt<T, Parser>::value (std::string const & v) {
            if (auto m = parser_ (v)) {
                value_ = m.value ();
                return true;
            }
            return false;
        }
        template <typename T, typename Parser>
        parser_base * opt<T, Parser>::get_parser () {
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
                apply_to_option (*this, mods...);
            }
            opt (opt const &) = delete;
            opt (opt &&) = delete;
            ~opt () noexcept override = default;

            opt & operator= (opt const &) = delete;
            opt & operator= (opt &&) = delete;

            explicit operator bool () const noexcept { return get (); }
            bool get () const noexcept { return value_; }

            bool takes_argument () const override { return false; }
            bool value (std::string const & v) override;
            bool add_occurrence () override;
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
                apply_to_option (*this, mods...);
            }

            list (list const &) = delete;
            list (list &&) = delete;
            ~list () noexcept override = default;

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

            gsl::czstring arg_description () const noexcept override {
                return type_description<T>::value;
            }

        private:
            bool comma_separated (std::string const & v);
            bool simple_value (std::string const & v);

            Parser parser_;
            std::list<T> values_;
        };

        // value
        // ~~~~~
        template <typename T, typename Parser>
        bool list<T, Parser>::value (std::string const & v) {
            if (this->allow_comma_separated ()) {
                return this->comma_separated (v);
            }

            return this->simple_value (v);
        }

        // comma separated
        // ~~~~~~~~~~~~~~~
        template <typename T, typename Parser>
        bool list<T, Parser>::comma_separated (std::string const & v) {
            for (auto const & subvalue : csv (v)) {
                if (!this->simple_value (subvalue)) {
                    return false;
                }
            }
            return true;
        }

        // simple value
        // ~~~~~~~~~~~~
        template <typename T, typename Parser>
        bool list<T, Parser>::simple_value (std::string const & v) {
            if (maybe<T> m = parser_ (v)) {
                values_.push_back (m.value ());
                return true;
            }
            return false;
        }

        template <typename T, typename Parser>
        parser_base * list<T, Parser>::get_parser () {
            return &parser_;
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
                apply_to_option (*this, mods...);
            }

            alias (alias const &) = delete;
            alias (alias &&) = delete;
            ~alias () noexcept override = default;

            alias & operator= (alias const &) = delete;
            alias & operator= (alias &&) = delete;

            alias * as_alias () override { return this; }
            alias const * as_alias () const override { return this; }

            option_category const * category () const noexcept override {
                return original_->category ();
            }

            bool add_occurrence () override;
            void set_num_occurrences_flag (num_occurrences_flag n) override;
            num_occurrences_flag get_num_occurrences_flag () const override;
            void set_positional () override;
            bool is_positional () const override;
            bool is_alias () const override;
            unsigned get_num_occurrences () const override;
            parser_base * get_parser () override;
            bool takes_argument () const override;
            bool value (std::string const & v) override;

            gsl::czstring arg_description () const noexcept override {
                return original_->arg_description ();
            }

            void set_original (option * o);
            option const * original () const { return original_; }

        private:
            option * original_ = nullptr;
        };

    } // end namespace command_line
} // end namespace pstore

#endif // PSTORE_COMMAND_LINE_OPTION_HPP
