//*             _             *
//* __   ____ _| |_   _  ___  *
//* \ \ / / _` | | | | |/ _ \ *
//*  \ V / (_| | | |_| |  __/ *
//*   \_/ \__,_|_|\__,_|\___| *
//*                           *
//===- lib/dump/value.cpp -------------------------------------------------===//
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
/// \file value.cpp

#include "pstore/dump/value.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <cwctype>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <tuple>

#include "pstore/support/array_elements.hpp"
#include "pstore/support/utf.hpp"
#include "pstore/support/time.hpp"

namespace {

    template <typename CharType>
    CharType to_hex (unsigned v) {
        assert (v < 0x10);
        return static_cast<CharType> (v + ((v < 10) ? '0' : 'A' - 10));
    }

} // end anonymous namespace

namespace pstore {
    namespace dump {

        template <typename OStream>
        OStream & indent::write (OStream & os, typename OStream::char_type c) const {
            for (auto ctr = 0U; ctr < count_; ++ctr) {
                os << c;
            }
            return os;
        }

        template <typename CharType>
        std::basic_string<CharType> indent::str () const {
            using string_type = std::basic_string<CharType>;
            typename string_type::size_type n = count_;
            typename string_type::value_type c = ' ';
            return string_type (n, c);
        }

        std::ostream & operator<< (std::ostream & os, indent const & ind) {
            return ind.write (os, ' ');
        }
        std::wostream & operator<< (std::wostream & os, indent const & ind) {
            return ind.write (os, L' ');
        }



        //*****************
        //*   v a l u e   *
        //*****************

        // (dtor)
        // ~~~~~~
        value::~value () {}

        // write
        // ~~~~~
        std::ostream & value::write (std::ostream & os) const {
            return this->write_impl (os, indent ());
        }
        std::wostream & value::write (std::wostream & os) const {
            return this->write_impl (os, indent ());
        }


        //*******************
        //*   s t r i n g   *
        //*******************

        // should_be_quoted
        // ~~~~~~~~~~~~~~~~
        bool string::should_be_quoted (std::string const & v) {
            using uchar_type = typename std::make_unsigned<std::string::value_type>::type;

            auto predicate = [](char c) { return !std::isprint (static_cast<uchar_type> (c)); };
            return v.length () == 0 || std::isspace (static_cast<uchar_type> (v.front ())) ||
                   std::isspace (static_cast<uchar_type> (v.back ())) ||
                   std::find_if (std::begin (v), std::end (v), predicate) != std::end (v);
        }

        // write_unquoted
        // ~~~~~~~~~~~~~~
        template <typename OStream>
        OStream & string::write_unquoted (OStream & os, std::string const & v) {
            // If the string starts with backslash or bang, prefix with a backslash
            if (v.length () > 0) {
                auto const front = v.front ();
                if (front == '\"' || front == '!') {
                    os << "\\";
                }
            }
            // Emit the characters; a backslash is escaped.
            for (char ch : v) {
                if (ch == '\\') {
                    os << '\\';
                }
                os << ch;
            }
            return os;
        }

        // write_codepoint_hex
        // ~~~~~~~~~~~~~~~~~~~
        template <typename OStream, typename UCharType>
        OStream & string::write_codepoint_hex (OStream & os, UCharType ch) {
            static_assert (std::is_unsigned<UCharType>::value, "UCharType must be unsigned");
            return os << "\\x" << to_hex<UCharType> ((ch >> 4U) & 0x0FU)
                      << to_hex<UCharType> (ch & 0x0FU);
        }

        // write_character
        // ~~~~~~~~~~~~~~~
        template <typename OStream>
        OStream & string::write_character (OStream & os, char ch) {
            // Control characters are U+0000 through U+001F. Allow everything
            // else through as UTF-8.
            auto const uch = static_cast<unsigned char> (ch);
            if (uch < 128U && std::isprint (static_cast<int> (ch))) {
                os << ch;
            } else {
                write_codepoint_hex (os, uch);
            }
            return os;
        }

        // write_quoted
        // ~~~~~~~~~~~~
        template <typename OStream>
        OStream & string::write_quoted (OStream & os, std::string const & v) {
            os << "\"";
            for (char const ch : v) {
                switch (ch) {
                case '\0': os << "\\0"; break;
                case '\x7': os << "\\a"; break;
                case '\x8': os << "\\b"; break;
                case '\x9': os << "\\t"; break;
                case '\xa': os << "\\n"; break;
                case '\xb': os << "\\v"; break;
                case '\xc': os << "\\f"; break;
                case '\xd': os << "\\r"; break;
                case '\x1b': os << "\\e"; break;
                case '"': os << "\\\""; break;
                default: write_character (os, ch); break;
                }
            }
            os << "\"";
            return os;
        }

        // writer
        // ~~~~~~
        template <typename OStream>
        OStream & string::writer (OStream & os) const {
            if (!force_quoted_ && !should_be_quoted (v_)) {
                return write_unquoted (os, v_);
            }
            return write_quoted (os, v_);
        }

        // write_impl
        // ~~~~~~~~~~
        std::ostream & string::write_impl (std::ostream & os, indent const &) const {
            return this->writer (os);
        }
        std::wostream & string::write_impl (std::wostream & os, indent const &) const {
            return this->writer (os);
        }


        //*****************************
        //*   n u m b e r _ b a s e   *
        //*****************************
        unsigned number_base::default_base_ = 16;

        // (dtor)
        // ~~~~~~
        number_base::~number_base () {}


        //*********************************
        //*   n u m b e r _ d o u b l e   *
        //*********************************
        std::ostream & number_double::write_impl (std::ostream & os, indent const &) const {
            return os << v_;
        }
        std::wostream & number_double::write_impl (std::wostream & os, indent const &) const {
            return os << v_;
        }

        //*****************************
        //*   n u m b e r _ l o n g   *
        //*****************************
        std::ostream & number_long::write_impl (std::ostream & os, indent const &) const {
            return this->write_decimal (v_, os);
        }
        std::wostream & number_long::write_impl (std::wostream & os, indent const &) const {
            return this->write_decimal (v_, os);
        }

        //*******************************
        //*   n u m b e r _ u l o n g   *
        //*******************************
        std::ostream & number_ulong::write_impl (std::ostream & os, indent const &) const {
            return this->write_decimal (v_, os);
        }
        std::wostream & number_ulong::write_impl (std::wostream & os, indent const &) const {
            return this->write_decimal (v_, os);
        }

        //*********************
        //*   b o o l e a n   *
        //*********************
        std::ostream & boolean::write_impl (std::ostream & os, indent const & /*indent*/) const {
            return os << (v_ ? "true" : "false");
        }
        std::wostream & boolean::write_impl (std::wostream & os, indent const & /*indent*/) const {
            return os << (v_ ? L"true" : L"false");
        }

        //***************
        //*   n u l l   *
        //***************
        std::ostream & null::write_impl (std::ostream & os, indent const & /*indent*/) const {
            return os << "null";
        }
        std::wostream & null::write_impl (std::wostream & os, indent const & /*indent*/) const {
            return os << L"null";
        }


        //***************
        //*   t i m e   *
        //***************
        // milliseconds_to_time
        // ~~~~~~~~~~~~~~~~~~~~
        std::time_t time::milliseconds_to_time () const {
            using clock = std::chrono::system_clock;
            clock::duration dur = std::chrono::milliseconds (ms_);
            std::chrono::time_point<clock> tp (dur);
            return std::chrono::system_clock::to_time_t (tp);
        }

        // writer
        // ~~~~~~
        template <typename OStream>
        OStream & time::writer (OStream & os) const {
            bool fallback = true;
            std::time_t t = this->milliseconds_to_time ();
            std::tm const tm = gm_time (t);
            char time_str[100];
            // strftime() returns 0 if the result doesn't fit in the provided buffer;
            // the contents are undefined.
            // TODO: use small_vector? 100 characters should be plenty for an ISO8601 date.
            if (std::strftime (time_str, sizeof (time_str), "%FT%TZ", &tm) != 0) {
                // Guarantee that the string is null terminated.
                time_str[array_elements (time_str) - 1] = '\0';
                os << time_str;
                fallback = false;
            }

            if (fallback) {
                os << "Time(" << ms_ << "ms)";
            }
            return os;
        }

        // write_impl
        // ~~~~~~~~~~
        std::ostream & time::write_impl (std::ostream & os, indent const & /*indent*/) const {
            return this->writer (os);
        }
        std::wostream & time::write_impl (std::wostream & os, indent const & /*indent*/) const {
            return this->writer (os);
        }


        //*****************
        //*   a r r a y   *
        //*****************
        // writer
        // ~~~~~~
        template <typename OStream>
        OStream & array::writer (OStream & os, indent const & ind) const {
            char const * sep = "";

            bool const is_compact = values_.size () == 0 || this->is_number_array ();
            if (is_compact) {
                os << "[";
                sep = " ";
                for (value_ptr const & value : values_) {
                    os << sep;

                    auto v = value.get ();
                    v->write_impl (os, indent ());

                    sep = ", ";
                }
                os << " ]";
            } else {
                os << '\n';
                indent next_indent = ind.next (2);
                for (value_ptr const & value : values_) {
                    os << sep << ind << "- ";
                    if (value->dynamic_cast_object () != nullptr) {
                        next_indent = ind.next (2);
                    } else {
                        next_indent = ind.next (4);
                    }

                    value->write_impl (os, next_indent);
                    sep = "\n";
                }
            }
            return os;
        }

        // write_impl
        // ~~~~~~~~~~
        std::ostream & array::write_impl (std::ostream & os, indent const & ind) const {
            return this->writer (os, ind);
        }
        std::wostream & array::write_impl (std::wostream & os, indent const & ind) const {
            return this->writer (os, ind);
        }

        // is_number_array
        // ~~~~~~~~~~~~~~~
        bool array::is_number_array () const {
            for (value_ptr const & value : values_) {
                if (!value->is_number_like ()) {
                    return false;
                }
            }
            return true;
        }


        //*******************
        //*   o b j e c t   *
        //*******************
        // property
        // ~~~~~~~~
        std::shared_ptr<string> object::property (std::string const & p) {
            // If the string contains ': ', then we _must_ quote it.
            bool const force_quoted = (p.find (": ") != std::string::npos);
            return std::make_shared<string> (p, force_quoted);
        }


        // property_length
        // ~~~~~~~~~~~~~~~
        /// \brief Used to measure key length so that we can align the output nicely.
        template <typename OStreamCharType>
        std::size_t object::property_length (std::string const & p) {
            std::basic_ostringstream<OStreamCharType> out;
            object::property (p)->write (out);
            return out.str ().length ();
        }


        // writer
        // ~~~~~~
        template <typename OStream, typename ObjectCharacterTraits>
        OStream & object::writer (OStream & os, indent const & ind,
                                  ObjectCharacterTraits const & traits) const {
            static_assert (
                std::is_same<typename OStream::char_type,
                             typename ObjectCharacterTraits::char_type>::value,
                "Object character traits char_type does not match stream character type");
            static object_strings<ObjectCharacterTraits> const strings{traits};

            bool const is_compact = compact_ || members_.size () == 0;
            if (is_compact) {
                return this->write_compact (os, strings);
            }
            return this->write_full_size (os, ind, strings);
        }

        // write_compact
        // ~~~~~~~~~~~~~
        template <typename OStream, typename ObjectCharacterTraits>
        OStream &
        object::write_compact (OStream & os,
                               object_strings<ObjectCharacterTraits> const & strings) const {
            os << strings.openbrace_str;
            auto const * separator = &strings.space_str;
            for (object::member const & kvp : members_) {
                os << *separator;
                object::property (kvp.property)->write (os);
                os << strings.colon_space_str;
                kvp.val->write (os);
                separator = &strings.comma_space_str;
            }
            os << strings.space_closebrace_str;
            return os;
        }

        // write_full_size
        // ~~~~~~~~~~~~~~~
        template <typename OStream, typename ObjectCharacterTraits>
        OStream &
        object::write_full_size (OStream & os, indent const & ind,
                                 object_strings<ObjectCharacterTraits> const & strings) const {
            using char_type = typename OStream::char_type;
            using string_type = std::basic_string<char_type>;

            // Proceed in two phases: the first measures the length of all of the object keys to
            // find
            // the longest. This enables the values to be nicely aligned by inserting the correct
            // horizontal spacing.
            enum { key_index, value_index, key_length_index };
            std::vector<std::tuple<value_ptr, value_ptr, std::size_t>> kvps;
            kvps.reserve (members_.size ());

            auto longest = std::size_t{0};
            for (object::member const & kvp : members_) {
                std::size_t const key_length = object::property_length<char_type> (kvp.property);
                longest = std::max (longest, key_length);
                kvps.emplace_back (object::property (kvp.property), kvp.val, key_length);
            }

            // The second phase: produce the keys and values with the correct number of spaces
            // between
            // them.
            string_type sep;
            indent next_indent = ind.next (4);

            for (auto const & a : kvps) {
                std::size_t const num_spaces = longest - std::get<key_length_index> (a) + 1U;
                os << sep << (*std::get<key_index> (a))
                   << string_type (num_spaces, strings.space_str[0]) << strings.colon_space_str;

                // If the value is an object then we may need to insert a new line here.
                auto value = std::get<value_index> (a);
                if (auto nested_obj = value->dynamic_cast_object ()) {
                    if (!nested_obj->is_compact ()) {
                        os << strings.cr_str << next_indent;
                    }
                }
                value->write_impl (os, next_indent);

                sep = strings.cr_str + ind.str<char_type> ();
            }
            return os;
        }

        // get
        // ~~~
        value_ptr object::get (std::string const & name) {
            auto predicate = [&name](object::member const & mem) { return mem.property == name; };
            auto it = std::find_if (std::begin (members_), std::end (members_), predicate);
            return (it != members_.end ()) ? it->val : value_ptr (nullptr);
        }

        // write_impl
        // ~~~~~~~~~~
        std::ostream & object::write_impl (std::ostream & os, indent const & indent) const {
            return this->writer (os, indent, object_char_traits ());
        }
        std::wostream & object::write_impl (std::wostream & os, indent const & indent) const {
            return this->writer (os, indent, object_wchar_traits ());
        }


        // object::member/
        object::member::member (std::string const & p, value_ptr const & v)
                : property (p)
                , val (v) {
            assert (v.get () != nullptr);
        }

        object::member::member (member && rhs)
                : property (std::move (rhs.property))
                , val (std::move (rhs.val)) {}

    } // namespace dump
} // namespace pstore

namespace {
    // We accumulate a number of lines of encoded binary in a single string object
    // rather than writing individual characters to the output stream one at a time.
    template <typename OStream>
    struct accumulator {
        accumulator (OStream & os, pstore::dump::indent const & ind);

        /// Append a single character to the output.
        void write (char c);

        /// Flush any remaining output.
        void flush ();

    private:
        static constexpr unsigned line_width_ = 80U;
        static constexpr unsigned num_lines_to_accumulate_ = 50U;

        OStream & os_;
        unsigned const chars_to_reserve_;

        using char_type = typename OStream::char_type;
        std::basic_string<char_type> const indent_string_;
        std::size_t width_ = 0;
        std::size_t line_count_ = 0;
        std::basic_string<char_type> lines_;
    };

    template <typename OStream>
    accumulator<OStream>::accumulator (OStream & os, pstore::dump::indent const & ind)
            : os_{os}
            , chars_to_reserve_{(ind.size () + line_width_ + 1U) * num_lines_to_accumulate_}
            , indent_string_{ind.str<char_type> ()} {
        lines_.reserve (chars_to_reserve_);
    }

    template <typename OStream>
    void accumulator<OStream>::write (char c) {
        if (width_ == 0) {
            lines_ += indent_string_;
        }
        lines_ += c;
        if (++width_ >= line_width_) {
            lines_ += '\n';
            width_ = 0;
            if (++line_count_ >= num_lines_to_accumulate_) {
                os_ << lines_;
                line_count_ = 0;

                lines_.clear ();
                lines_.reserve (chars_to_reserve_);
            }
        }
    }

    template <typename OStream>
    void accumulator<OStream>::flush () {
        os_ << lines_;

        width_ = 0U;
        line_count_ = 0U;
        lines_.clear ();
    }
} // namespace

namespace pstore {
    namespace dump {

        //*******************
        //*   b i n a r y   *
        //*******************
        // writer
        // ~~~~~~
        template <typename OStream>
        OStream & binary::writer (OStream & os, indent const & ind) const {

            static constexpr char alphabet[]{"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                             "abcdefghijklmnopqrstuvwxyz"
                                             "0123456789+/"};

            accumulator<OStream> acc (os, ind);

            os << "!!binary |\n";

            std::uint32_t temp;
            auto it = std::begin (v_);
            std::size_t const size = v_.size ();
            for (std::size_t index = 0; index < size / 3U; ++index) {
                temp = static_cast<std::uint32_t> ((*it) << 16);
                ++it;
                temp += static_cast<std::uint32_t> ((*it) << 8);
                ++it;
                temp += (*it);
                ++it;

                acc.write (alphabet[(temp & 0x00FC0000) >> 18]);
                acc.write (alphabet[(temp & 0x0003F000) >> 12]);
                acc.write (alphabet[(temp & 0x00000FC0) >> 6]);
                acc.write (alphabet[temp & 0x0000003F]);
            }

            switch (size % 3) {
            case 0: break;

            case 1:
                temp = static_cast<std::uint32_t> ((*it++) << 16);
                acc.write (alphabet[(temp & 0x00FC0000) >> 18]);
                acc.write (alphabet[(temp & 0x0003F000) >> 12]);
                acc.write ('=');
                acc.write ('=');
                break;

            case 2:
                temp = static_cast<std::uint32_t> ((*it++) << 16);
                temp += static_cast<std::uint32_t> ((*it++) << 8);

                acc.write (alphabet[(temp & 0x00FC0000) >> 18]);
                acc.write (alphabet[(temp & 0x0003F000) >> 12]);
                acc.write (alphabet[(temp & 0x00000FC0) >> 6]);
                acc.write ('=');
                break;

            default: assert (false); break;
            }
            acc.flush ();
            assert (it == std::end (v_));
            return os;
        }

        // write_impl
        // ~~~~~~~~~~
        std::ostream & binary::write_impl (std::ostream & os, indent const & ind) const {
            return this->writer (os, ind);
        }
        std::wostream & binary::write_impl (std::wostream & os, indent const & ind) const {
            return this->writer (os, ind);
        }

    } // namespace dump
} // namespace pstore

namespace pstore {
    namespace dump {

        //**********************
        //*   b i n a r y 1 6  *
        //**********************
        // writer
        // ~~~~~~
        template <typename OStream>
        OStream & binary16::writer (OStream & os, indent const & ind) const {
            os << "!!binary16 |";
            if (v_.size () == 0) {
                os << '\n' << ind;
            } else {
                using char_type = typename OStream::char_type;
                constexpr auto number_of_bytes_per_row = 8U * 2U;
                auto ctr = 0U;
                for (std::uint8_t b : v_) {
                    if (ctr == 0U || ctr >= number_of_bytes_per_row) {
                        // Line wrap and indent on the first iteration or when we run out of width.
                        os << '\n' << ind;
                        ctr = 0U;
                    } else if (ctr % 2U == 0U) {
                        // Values grouped into two-byte pairs.
                        os << ' ';
                    }
                    char_type const hex[3] = {to_hex<char_type> ((b >> 4U) & 0x0FU),
                                              to_hex<char_type> (b & 0x0FU), '\0'};
                    os << hex;
                    ++ctr;
                }
            }
            return os << '>';
        }

        // write_impl
        // ~~~~~~~~~~
        std::ostream & binary16::write_impl (std::ostream & os, indent const & ind) const {
            return this->writer (os, ind);
        }
        std::wostream & binary16::write_impl (std::wostream & os, indent const & ind) const {
            return this->writer (os, ind);
        }

    } // namespace dump
} // namespace pstore
