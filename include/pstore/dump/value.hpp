//*             _             *
//* __   ____ _| |_   _  ___  *
//* \ \ / / _` | | | | |/ _ \ *
//*  \ V / (_| | | |_| |  __/ *
//*   \_/ \__,_|_|\__,_|\___| *
//*                           *
//===- include/pstore/dump/value.hpp --------------------------------------===//
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
/// \file value.hpp

#ifndef PSTORE_DUMP_VALUE_HPP
#define PSTORE_DUMP_VALUE_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <ios>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "pstore/support/ios_state.hpp"

namespace pstore {
    namespace dump {

        class indent {
        public:
            indent () = default;
            indent next (unsigned const distance) const noexcept {
                return indent{count_ + distance};
            }

            template <typename OStream>
            OStream & write (OStream & os, typename OStream::char_type c) const;

            template <typename CharType>
            std::basic_string<CharType> str () const;

            unsigned size () const { return count_; }

        private:
            explicit constexpr indent (unsigned const count) noexcept
                    : count_{count} {}
            unsigned count_ = 0U;
        };

        std::ostream & operator<< (std::ostream & os, indent const & ind);
        std::wostream & operator<< (std::wostream & os, indent const & ind);

        class array;
        class boolean;
        class object;
        class null;
        class number_long;
        class number_ulong;
        class number_double;
        class string;

        /// \brief The abstract base class for heterogeneous values.
        class value {
        public:
            virtual ~value () = 0;

            value () noexcept = default;
            value (value const &) noexcept = default;
            value & operator= (value const &) noexcept = default;

            ///@{
            virtual object * dynamic_cast_object () noexcept { return nullptr; }
            virtual object const * dynamic_cast_object () const noexcept { return nullptr; }
            virtual array * dynamic_cast_array () noexcept { return nullptr; }
            virtual array const * dynamic_cast_array () const noexcept { return nullptr; }

            virtual number_long * dynamic_cast_number_long () noexcept { return nullptr; }
            virtual number_long const * dynamic_cast_number_long () const noexcept {
                return nullptr;
            }
            virtual number_ulong * dynamic_cast_number_ulong () noexcept { return nullptr; }
            virtual number_ulong const * dynamic_cast_number_ulong () const noexcept {
                return nullptr;
            }
            virtual number_double * dynamic_cast_number_double () noexcept { return nullptr; }
            virtual number_double const * dynamic_cast_number_double () const noexcept {
                return nullptr;
            }

            virtual boolean * dynamic_cast_boolean () noexcept { return nullptr; }
            virtual boolean const * dynamic_cast_boolean () const noexcept { return nullptr; }
            virtual null * dynamic_cast_null () noexcept { return nullptr; }
            virtual null const * dynamic_cast_null () const noexcept { return nullptr; }
            virtual string * dynamic_cast_string () noexcept { return nullptr; }
            virtual string const * dynamic_cast_string () const noexcept { return nullptr; }
            ///@}


            virtual bool is_number_like () const { return false; }

            virtual std::ostream & write (std::ostream & os) const;
            virtual std::wostream & write (std::wostream & os) const;

            virtual std::ostream & write_impl (std::ostream & os, indent const & indent) const = 0;
            virtual std::wostream & write_impl (std::wostream & os,
                                                indent const & indent) const = 0;
        };

        using value_ptr = std::shared_ptr<value>;

        template <typename OStream>
        inline OStream & operator<< (OStream & os, value const & v) {
            v.write (os);
            return os;
        }


        /// \brief  An intermediate abstract base class for number types.
        class number_base : public value {
        public:
            ~number_base () override;

            bool is_number_like () const override { return true; }

            static void hex () noexcept { default_base_ = 16U; }
            static void dec () noexcept { default_base_ = 10U; }
            static void oct () noexcept { default_base_ = 8U; }

            static unsigned default_base () { return default_base_; }

        protected:
            number_base () noexcept
                    : base_{default_base_} {}
            explicit constexpr number_base (unsigned const base) noexcept
                    : base_{base} {}

            static unsigned default_base_;
            unsigned const base_;

            // write_decimal
            // ~~~~~~~~~~~~~
            template <typename T, typename OStream>
            OStream & write_decimal (T value, OStream & os) const {
                ios_flags_saver const _{os};
                switch (base_) {
                case 16: os << "0x" << std::hex; break;
                case 8:
                    if (value != 0) {
                        os << "0";
                    }
                    os << std::oct;
                    break;
                case 10: os << std::dec; break;
                default: assert (0); break;
                }
                return os << value;
            }
        };

        //*********************************
        //*   n u m b e r _ d o u b l e   *
        //*********************************
        class number_double final : public number_base {
        public:
            explicit number_double (double const v) noexcept
                    : number_base ()
                    , v_{v} {}
            double get () const noexcept { return v_; }

            number_double * dynamic_cast_number_double () noexcept override { return this; }
            number_double const * dynamic_cast_number_double () const noexcept override {
                return this;
            }

        private:
            std::ostream & write_impl (std::ostream & os, indent const & i) const override;
            std::wostream & write_impl (std::wostream & os, indent const & i) const override;

            double v_;
        };

        //*****************************
        //*   n u m b e r _ l o n g   *
        //*****************************
        class number_long final : public number_base {
        public:
            explicit number_long (long long const v) noexcept
                    : number_base ()
                    , v_{v} {}
            number_long (long long const v, unsigned const base)
                    : number_base (base)
                    , v_ (v) {}
            long long get () const noexcept { return v_; }

            number_long * dynamic_cast_number_long () noexcept override { return this; }
            number_long const * dynamic_cast_number_long () const noexcept override { return this; }

        private:
            std::ostream & write_impl (std::ostream & os, indent const & i) const override;
            std::wostream & write_impl (std::wostream & os, indent const & i) const override;

            long long v_;
        };

        //*******************************
        //*   n u m b e r _ u l o n g   *
        //*******************************
        class number_ulong final : public number_base {
        public:
            explicit number_ulong (unsigned long long const v) noexcept
                    : number_base ()
                    , v_{v} {}
            number_ulong (unsigned long long const v, unsigned const base)
                    : number_base (base)
                    , v_ (v) {}
            unsigned long long get () const noexcept { return v_; }

            number_ulong * dynamic_cast_number_ulong () noexcept override { return this; }
            number_ulong const * dynamic_cast_number_ulong () const noexcept override {
                return this;
            }

        private:
            std::ostream & write_impl (std::ostream & os, indent const & i) const override;
            std::wostream & write_impl (std::wostream & os, indent const & i) const override;

            unsigned long long v_;
        };


        //*********************
        //*   b o o l e a n   *
        //*********************
        /// \brief A class used to write a boolean values to an ostream.
        class boolean final : public value {
        public:
            explicit boolean (bool const v) noexcept
                    : v_{v} {}

            boolean * dynamic_cast_boolean () noexcept override { return this; }
            boolean const * dynamic_cast_boolean () const noexcept override { return this; }

            bool get () const noexcept { return v_; }

        private:
            std::ostream & write_impl (std::ostream & os, indent const & indent) const override;
            std::wostream & write_impl (std::wostream & os, indent const & indent) const override;
            bool v_;
        };

        //***************
        //*   n u l l   *
        //***************
        /// \brief A class used to denote a null value.
        class null final : public value {
        public:
            null () = default;

            null * dynamic_cast_null () noexcept override { return this; }
            null const * dynamic_cast_null () const noexcept override { return this; }

        private:
            std::ostream & write_impl (std::ostream & os, indent const & indent) const override;
            std::wostream & write_impl (std::wostream & os, indent const & indent) const override;
        };


        //*******************
        //*   s t r i n g   *
        //*******************
        /// \brief A class used to write strings.
        class string final : public value {
        public:
            /// Constructs a value string object.
            /// \param v The string represented by this value object.
            /// \param force_quoted  If true, forces the output string to be quoted.
            explicit string (std::string v, bool const force_quoted = false) noexcept
                    : v_ (std::move (v))
                    , force_quoted_ (force_quoted) {}

            string * dynamic_cast_string () noexcept override { return this; }
            string const * dynamic_cast_string () const noexcept override { return this; }

            std::string const & get () const noexcept { return v_; }

        private:
            template <typename OStream>
            OStream & writer (OStream & os) const;

            std::ostream & write_impl (std::ostream & os, indent const & indent) const override;
            std::wostream & write_impl (std::wostream & os, indent const & indent) const override;

            static bool should_be_quoted (std::string const & v);

            /// Writes a simple, unquoted, string to the output stream.
            /// \param os  The stream to which output will be written.
            /// \param v The string to be written.
            /// \returns A reference to the stream parameter.
            template <typename OStream>
            static OStream & write_unquoted (OStream & os, std::string const & v);

            template <typename OStream>
            static OStream & write_character (OStream & os, char ch);

            /// Writes a non-trivial string which contains non-printable characters. Simple escape
            /// characters are used where possible but Unicode code-points are converted to hex
            /// where necessary.
            ///
            /// \param os The stream to which output will be written.
            /// \param v The string to be written.
            /// \returns A reference to the stream parameter.
            template <typename OStream>
            static OStream & write_quoted (OStream & os, std::string const & v);

            template <typename OStream, typename UCharType = typename std::make_unsigned<
                                            typename OStream::char_type>::type>
            static OStream & write_codepoint_hex (OStream & os, UCharType ch);

            std::string v_;
            bool force_quoted_;
        };


        //*******************
        //*   b i n a r y   *
        //*******************
        class binary final : public value {
        public:
            template <typename InputIterator>
            binary (InputIterator begin, InputIterator end)
                    : v_ (begin, end) {}

        private:
            template <typename OStream>
            OStream & writer (OStream & os, indent const & ind) const;
            std::ostream & write_impl (std::ostream & os, indent const & indent) const override;
            std::wostream & write_impl (std::wostream & os, indent const & indent) const override;
            std::vector<std::uint8_t> v_;
        };


        //***********************
        //*   b i n a r y 1 6   *
        //***********************
        /// Dumps a block of binary data in hex format (tagged as "!!binary16 in the YAML output).
        class binary16 final : public value {
        public:
            template <typename InputIterator>
            binary16 (InputIterator begin, InputIterator end)
                    : v_ (begin, end) {}

        private:
            template <typename OStream>
            OStream & writer (OStream & os, indent const & ind) const;
            std::ostream & write_impl (std::ostream & os, indent const & indent) const override;
            std::wostream & write_impl (std::wostream & os, indent const & indent) const override;
            std::vector<std::uint8_t> v_;
        };


        //****************
        //*   t i m  e   *
        //****************
        /// \brief A class used to write an ISO 8601-compatible time
        /// Given the time as a number of milliseconds since the epoch.
        class time final : public value {
        public:
            explicit time (std::uint64_t const ms)
                    : ms_ (ms) {}

        private:
            std::ostream & write_impl (std::ostream & os, indent const & indent) const override;
            std::wostream & write_impl (std::wostream & os, indent const & indent) const override;

            template <typename OStream>
            OStream & writer (OStream & os) const;

            std::time_t milliseconds_to_time () const;
            std::uint64_t ms_;
        };


        //*****************
        //*   a r r a y   *
        //*****************
        /// \brief A class used to write array.
        class array final : public value {
        public:
            using container = std::vector<value_ptr>;

            array () = default;
            explicit array (container const & values)
                    : values_ (values) {}
            explicit array (container && values)
                    : values_ (std::move (values)) {}
            array (array && rhs) = default;
            array (array const & rhs) = delete;

            array & operator= (array const &) = delete;
            array & operator= (array &&) = delete;

            array * dynamic_cast_array () noexcept override { return this; }
            array const * dynamic_cast_array () const noexcept override { return this; }

            void push_back (value_ptr const & v) { values_.push_back (v); }

            std::size_t size () const noexcept { return values_.size (); }
            value_ptr operator[] (std::size_t const index) noexcept { return values_[index]; }

        private:
            template <typename OStream>
            OStream & writer (OStream & os, indent const & ind) const;

            std::ostream & write_impl (std::ostream & os, indent const & ind) const override;
            std::wostream & write_impl (std::wostream & os, indent const & ind) const override;
            bool is_number_array () const;

            container values_;
        };

        using array_ptr = std::shared_ptr<array>;


        //*******************
        //*   o b j e c t   *
        //*******************

        struct object_char_traits {
            using char_type = char;
            static constexpr char const closebrace = '}';
            static constexpr char const colon = ':';
            static constexpr char const comma = ',';
            static constexpr char const cr = '\n';
            static constexpr char const openbrace = '{';
            static constexpr char const space = ' ';
        };
        struct object_wchar_traits {
            using char_type = wchar_t;
            static constexpr wchar_t const closebrace = L'}';
            static constexpr wchar_t const colon = L':';
            static constexpr wchar_t const comma = L',';
            static constexpr wchar_t const cr = L'\n';
            static constexpr wchar_t const openbrace = L'{';
            static constexpr wchar_t const space = L' ';
        };

        /// \brief A class used to write objects (key/value pairs).
        class object final : public value {
        public:
            struct member {
                member (std::string const & p, value_ptr const & v);
                member (member const &) = default;
                member (member && rhs);
                member & operator= (member const &) = default;

                std::string property;
                value_ptr val;
            };

            using container = std::vector<member>;

            object () = default;
            explicit object (container && m)
                    : members_ (std::move (m)) {}
            explicit object (container const & m)
                    : members_ (m) {}
            object (object && rhs) = default;
            object (object const & rhs) = delete;

            object & operator= (object const &) = delete;
            object & operator= (object &&) = delete;

            object * dynamic_cast_object () noexcept override { return this; }
            object const * dynamic_cast_object () const noexcept override { return this; }

            value_ptr get (std::string const & name);
            std::size_t size () const noexcept { return members_.size (); }

            void insert (std::string name, value_ptr value) {
                members_.emplace_back (std::move (name), std::move (value));
            }

            void compact (bool const enabled = true) noexcept { compact_ = enabled; }
            bool is_compact () const noexcept { return compact_; }

        private:
            template <typename OStream, typename ObjectCharacterTraits>
            OStream & writer (OStream & os, indent const & indent,
                              ObjectCharacterTraits const & traits) const;

            template <typename ObjectCharacterTraits>
            struct object_strings {
                using char_type = typename ObjectCharacterTraits::char_type;
                using string_type = std::basic_string<char_type>;

                explicit object_strings (ObjectCharacterTraits const & traits) noexcept
                        : space_str ({traits.space})
                        , cr_str ({traits.cr})
                        , colon_space_str ({traits.colon, traits.space})
                        , comma_space_str ({traits.comma, traits.space})
                        , openbrace_str ({traits.openbrace})
                        , space_closebrace_str ({traits.space, traits.closebrace}) {}

                string_type const space_str;
                string_type const cr_str;
                string_type const colon_space_str;
                string_type const comma_space_str;
                string_type const openbrace_str;
                string_type const space_closebrace_str;
            };

            std::ostream & write_impl (std::ostream & os, indent const & indent) const override;
            std::wostream & write_impl (std::wostream & os, indent const & indent) const override;
            static std::shared_ptr<string> property (std::string const & p);
            template <typename OStreamCharType>
            static std::size_t property_length (std::string const & p);

            /// \brief Writes a compact description of the object.
            /// \param os  The stream to which the output will be written.
            /// \param strings  A collection of strings that can be used to separate tokens in the
            /// output.
            /// \returns A reference to the output stream (os).
            template <typename OStream, typename ObjectCharacterTraits>
            OStream & write_compact (OStream & os,
                                     object_strings<ObjectCharacterTraits> const & strings) const;

            template <typename OStream, typename ObjectCharacterTraits>
            OStream & write_full_size (OStream & os, indent const & indx,
                                       object_strings<ObjectCharacterTraits> const & strings) const;

            container members_;
            bool compact_{false};
        };


        inline std::shared_ptr<number_long> make_number (long long v) {
            return std::make_shared<number_long> (v);
        }
        inline std::shared_ptr<number_long> make_number (long v) {
            return std::make_shared<number_long> (v);
        }
        inline std::shared_ptr<number_long> make_number (int v) {
            return std::make_shared<number_long> (v);
        }
        inline std::shared_ptr<number_long> make_number (short v) {
            return std::make_shared<number_long> (v);
        }
        inline std::shared_ptr<number_long> make_number (char v) {
            return std::make_shared<number_long> (v);
        }

        inline std::shared_ptr<number_ulong> make_number (unsigned long long v) {
            return std::make_shared<number_ulong> (v);
        }
        inline std::shared_ptr<number_ulong> make_number (unsigned long v) {
            return std::make_shared<number_ulong> (v);
        }
        inline std::shared_ptr<number_ulong> make_number (unsigned int v) {
            return std::make_shared<number_ulong> (v);
        }
        inline std::shared_ptr<number_ulong> make_number (unsigned short v) {
            return std::make_shared<number_ulong> (v);
        }
        inline std::shared_ptr<number_ulong> make_number (unsigned char v) {
            return std::make_shared<number_ulong> (v);
        }

        inline std::shared_ptr<number_double> make_number (float v) {
            return std::make_shared<number_double> (v);
        }
        inline std::shared_ptr<number_double> make_number (double v) {
            return std::make_shared<number_double> (v);
        }


        inline value_ptr make_time (std::uint64_t ms, bool const no_times) {
            if (no_times) {
                ms = 0;
            }
            return std::static_pointer_cast<value> (std::make_shared<time> (ms));
        }

        /// \brief  Makes a value object which represents a boolean quanitity.
        inline value_ptr make_value (bool v) {
            return std::static_pointer_cast<value, boolean> (std::make_shared<boolean> (v));
        }

        inline value_ptr make_value (std::uint8_t const & v) {
            return std::static_pointer_cast<value, number_ulong> (make_number (v));
        }
        inline value_ptr make_value (std::uint16_t const & v) {
            return std::static_pointer_cast<value, number_ulong> (make_number (v));
        }
        inline value_ptr make_value (std::uint32_t const & v) {
            return std::static_pointer_cast<value, number_ulong> (make_number (v));
        }
        inline value_ptr make_value (std::uint64_t const & v) {
            return std::static_pointer_cast<value, number_ulong> (make_number (v));
        }
        inline value_ptr make_value (float const & v) {
            return std::static_pointer_cast<value, number_double> (make_number (v));
        }
        inline value_ptr make_value (double const & v) {
            return std::static_pointer_cast<value, number_double> (make_number (v));
        }

        /// \brief  Makes a value object which represents a null-terminated character array.
        inline value_ptr make_value (char const * str) {
            return std::static_pointer_cast<value, string> (std::make_shared<string> (str));
        }
        /// \brief  Makes a value object which represents a string.
        inline value_ptr make_value (std::string const & str) {
            return std::static_pointer_cast<value, string> (std::make_shared<string> (str));
        }

        /// \brief  Makes a value object which represents an object (an ordered collection of
        /// key/value
        ///         pairs).
        inline value_ptr make_value (object::container && members) {
            return std::static_pointer_cast<value, object> (
                std::make_shared<object> (std::move (members)));
        }
        inline value_ptr make_value (object::container const & members) {
            return std::static_pointer_cast<value, object> (std::make_shared<object> (members));
        }

        /// \brief  Makes a value object which represents a heterogeneous array from a collection of
        /// value objects.
        inline value_ptr make_value (array::container && members) {
            return std::static_pointer_cast<value, array> (
                std::make_shared<array> (std::move (members)));
        }
        inline value_ptr make_value (array::container const & members) {
            return std::static_pointer_cast<value, array> (std::make_shared<array> (members));
        }

        template <typename T, size_t Size>
        inline value_ptr make_value (std::array<T, Size> const & arr) {
            array::container contents;
            contents.reserve (Size);
            std::transform (std::begin (arr), std::end (arr), std::back_inserter (contents),
                            [](T const & in) { return make_value (in); });
            return make_value (std::move (contents));
        }


        using members = object::container;

    } // namespace dump
} // namespace pstore
#endif /* defined(PSTORE_DUMP_VALUE_HPP) */
