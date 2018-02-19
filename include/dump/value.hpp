//*             _             *
//* __   ____ _| |_   _  ___  *
//* \ \ / / _` | | | | |/ _ \ *
//*  \ V / (_| | | |_| |  __/ *
//*   \_/ \__,_|_|\__,_|\___| *
//*                           *
//===- include/dump/value.hpp ---------------------------------------------===//
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

#include "pstore/serialize/ios_state.hpp"

namespace pstore {
    namespace dump {

        class indent {
        public:
            indent () = default;
            indent next (unsigned distance) const { return indent{count_ + distance}; }

            template <typename OStream>
            OStream & write (OStream & os, typename OStream::char_type c) const;

            template <typename CharType>
            std::basic_string<CharType> str () const;

            unsigned size () const { return count_; }

        private:
            explicit indent (unsigned count)
                    : count_ (count) {}
            unsigned count_ = 0U;
        };

        std::ostream & operator<< (std::ostream & os, indent const & ind);
        std::wostream & operator<< (std::wostream & os, indent const & ind);



        class object;
        class number_base;

        /// \brief The abstract base class for heterogeneous values.
        class value {
        public:
            virtual ~value () = 0;

            value () = default;
            value (value const &) = default;
            value & operator= (value const &) = default;

            virtual object * dynamic_cast_object () { return nullptr; }
            virtual object const * dynamic_cast_object () const { return nullptr; }
            virtual number_base * dynamic_cast_number () { return nullptr; }
            virtual number_base const * dynamic_cast_number () const { return nullptr; }

            virtual bool is_number_like () const { return false; }

            virtual std::ostream & write (std::ostream & os) const;
            virtual std::wostream & write (std::wostream & os) const;

            virtual std::ostream & write_impl (std::ostream & os, indent const & indent) const = 0;
            virtual std::wostream & write_impl (std::wostream & os,
                                                indent const & indent) const = 0;
        };

        typedef std::shared_ptr<value> value_ptr;

        template <typename OStream>
        inline OStream & operator<< (OStream & os, value const & v) {
            v.write (os);
            return os;
        }


        /// \brief  An intermediate abstract base class for number types.
        class number_base : public value {
        public:
            ~number_base () override;

            number_base * dynamic_cast_number () override { return this; }
            number_base const * dynamic_cast_number () const override { return this; }

            bool is_number_like () const override { return true; }

            static void hex () { default_base_ = 16U; }
            static void dec () { default_base_ = 10U; }
            static void oct () { default_base_ = 8U; }

            static unsigned default_base () { return default_base_; }

        protected:
            number_base ()
                    : base_ (default_base_) {}
            explicit number_base (unsigned base)
                    : base_ (base) {}

            static unsigned default_base_;
            unsigned const base_;
        };


        //*******************
        //*   n u m b e r   *
        //*******************
        /// \brief  The class used to write numbers to an ostream.
        template <typename Ty>
        class number final : public number_base {
        public:
            explicit number (Ty v)
                    : number_base ()
                    , v_ (v) {}
            number (Ty v, unsigned base)
                    : number_base (base)
                    , v_ (v) {}

            template <typename OStream>
            OStream & write_fp (OStream & os, indent const & indent) const;
            template <typename OStream>
            OStream & write_decimal (OStream & os, indent const & indent) const;

        private:
            std::ostream & write_impl (std::ostream & os, indent const & indent) const override;
            std::wostream & write_impl (std::wostream & os, indent const & indent) const override;

            Ty v_;
        };

        // write_decimal
        // ~~~~~~~~~~~~~
        template <typename T>
        template <typename OStream>
        OStream & number<T>::write_decimal (OStream & os, indent const &) const {
            serialize::ios_flags_saver old_flags (os);
            switch (base_) {
            case 16: os << "0x" << std::hex; break;
            case 8:
                if (v_ != 0) {
                    os << "0";
                }
                os << std::oct;
                break;
            case 10: os << std::dec; break;
            default: assert (0); break;
            }
            os << v_;
            return os;
        }

        // write_fp
        // ~~~~~~~~
        template <typename T>
        template <typename OStream>
        inline OStream & number<T>::write_fp (OStream & os, indent const &) const {
            return os << v_;
        }

        // write_impl
        // ~~~~~~~~~~
        template <typename Ty>
        std::ostream & number<Ty>::write_impl (std::ostream & os, indent const & indent) const {
            return this->write_decimal (os, indent);
        }
        template <typename Ty>
        std::wostream & number<Ty>::write_impl (std::wostream & os, indent const & indent) const {
            return this->write_decimal (os, indent);
        }

        template <>
        inline std::ostream & number<std::uint8_t>::write_impl (std::ostream & os,
                                                                indent const & indent) const {
            return number<std::uint16_t> (v_).write_decimal (os, indent);
        }
        template <>
        inline std::wostream & number<std::uint8_t>::write_impl (std::wostream & os,
                                                                 indent const & indent) const {
            return number<std::uint16_t> (v_).write_decimal (os, indent);
        }
        template <>
        inline std::ostream & number<int>::write_impl (std::ostream & os,
                                                       indent const & indent) const {
            return this->write_decimal (os, indent);
        }
        template <>
        inline std::wostream & number<int>::write_impl (std::wostream & os,
                                                        indent const & indent) const {
            return this->write_decimal (os, indent);
        }
        template <>
        inline std::ostream & number<float>::write_impl (std::ostream & os,
                                                         indent const & indent) const {
            return this->write_fp (os, indent);
        }
        template <>
        inline std::wostream & number<float>::write_impl (std::wostream & os,
                                                          indent const & indent) const {
            return this->write_fp (os, indent);
        }
        template <>
        inline std::ostream & number<double>::write_impl (std::ostream & os,
                                                          indent const & indent) const {
            return this->write_fp (os, indent);
        }
        template <>
        inline std::wostream & number<double>::write_impl (std::wostream & os,
                                                           indent const & indent) const {
            return this->write_fp (os, indent);
        }
        template <>
        inline std::ostream & number<long double>::write_impl (std::ostream & os,
                                                               indent const & indent) const {
            return this->write_fp (os, indent);
        }
        template <>
        inline std::wostream & number<long double>::write_impl (std::wostream & os,
                                                                indent const & indent) const {
            return this->write_fp (os, indent);
        }


        //*********************
        //*   b o o l e a n   *
        //*********************
        /// \brief A class used to write a boolean values to an ostream.
        class boolean final : public value {
        public:
            explicit boolean (bool v)
                    : v_ (v) {}

        private:
            std::ostream & write_impl (std::ostream & os, indent const & indent) const override;
            std::wostream & write_impl (std::wostream & os, indent const & indent) const override;
            bool v_;
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
            explicit string (std::string v, bool force_quoted = false)
                    : v_ (std::move (v))
                    , force_quoted_ (force_quoted) {}

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
            /// where
            /// necessary.
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
            explicit binary (InputIterator begin, InputIterator end)
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
            explicit binary16 (InputIterator begin, InputIterator end)
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
            explicit time (std::uint64_t ms)
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
            array (container const & values)
                    : values_ (values) {}
            array (container && values)
                    : values_ (std::move (values)) {}
            array (array && rhs) = default;
            array (array const & rhs) = delete;

            array & operator= (array const &) = delete;
            array & operator= (array &&) = delete;

            void push_back (value_ptr v) { values_.push_back (v); }

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
            object (container && m)
                    : members_ (std::move (m)) {}
            object (container const & m)
                    : members_ (m) {}
            object (object && rhs) = default;
            object (object const & rhs) = delete;

            object & operator= (object const &) = delete;
            object & operator= (object &&) = delete;

            object * dynamic_cast_object () override { return this; }
            object const * dynamic_cast_object () const override { return this; }

            value_ptr get (std::string const & name);

            void compact (bool enabled = true) { compact_ = enabled; }
            bool is_compact () const { return compact_; }

        private:
            template <typename OStream, typename ObjectCharacterTraits>
            OStream & writer (OStream & os, indent const & indent,
                              ObjectCharacterTraits const & traits) const;

            template <typename ObjectCharacterTraits>
            struct object_strings {
                using char_type = typename ObjectCharacterTraits::char_type;
                using string_type = std::basic_string<char_type>;

                explicit object_strings (ObjectCharacterTraits const & traits) noexcept
                        : space_str{{traits.space}}
                        , cr_str{{traits.cr}}
                        , colon_space_str{{traits.colon, traits.space}}
                        , comma_space_str{{traits.comma, traits.space}}
                        , openbrace_str{{traits.openbrace}}
                        , space_closebrace_str{{traits.space, traits.closebrace}} {}

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


        template <typename T>
        inline std::shared_ptr<number<T>> make_number (T const & t) {
            return std::make_shared<number<T>> (t);
        }

        inline value_ptr make_time (std::uint64_t ms, bool no_times) {
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
            return std::static_pointer_cast<value, number<std::uint8_t>> (make_number (v));
        }
        inline value_ptr make_value (std::uint16_t const & v) {
            return std::static_pointer_cast<value, number<std::uint16_t>> (make_number (v));
        }
        inline value_ptr make_value (std::uint32_t const & v) {
            return std::static_pointer_cast<value, number<std::uint32_t>> (make_number (v));
        }
        inline value_ptr make_value (std::uint64_t const & v) {
            return std::static_pointer_cast<value, number<std::uint64_t>> (make_number (v));
        }
        inline value_ptr make_value (float const & v) {
            return std::static_pointer_cast<value, number<float>> (make_number (v));
        }
        inline value_ptr make_value (double const & v) {
            return std::static_pointer_cast<value, number<double>> (make_number (v));
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
            auto v = std::make_shared<object> (std::move (members));
            return std::static_pointer_cast<value, object> (v);
        }
        inline value_ptr make_value (object::container const & members) {
            auto v = std::make_shared<object> (members);
            return std::static_pointer_cast<value, object> (v);
        }

        /// \brief  Makes a value object which represents a heterogeneous array from a collection of
        /// value objects.
        inline value_ptr make_value (array::container && members) {
            auto v = std::make_shared<array> (std::move (members));
            return std::static_pointer_cast<value, array> (v);
        }
        inline value_ptr make_value (array::container const & members) {
            auto v = std::make_shared<array> (members);
            return std::static_pointer_cast<value, array> (v);
        }

        template <typename T, size_t Size>
        inline value_ptr make_value (std::array<T, Size> const & arr) {
            array::container contents;
            contents.reserve (Size);
            std::transform (std::begin (arr), std::end (arr), std::back_inserter (contents),
                            [](T const & in) -> value_ptr { return make_value (in); });
            return make_value (std::move (contents));
        }


        using members = object::container;

    } // namespace dump
} // namespace pstore
#endif /* defined(PSTORE_DUMP_VALUE_HPP) */
// eof: include/dump/value.hpp
