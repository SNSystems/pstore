//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
//===- include/pstore_json/json.hpp ---------------------------------------===//
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
#ifndef PSTORE_JSON_HPP
#define PSTORE_JSON_HPP

#include <cstdint>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <stack>
#include <system_error>

#include "pstore_json/json_error.hpp"
#include "pstore_json/utf.hpp"
#include "pstore_support/maybe.hpp"
#include "pstore_support/portab.hpp"

namespace pstore {
    namespace json {

        class source;
        template <typename Callbacks>
        class matcher;

        template <typename Callbacks>
        class parser {
        public:
            using result_type = typename Callbacks::result_type;

            parser (Callbacks const & callbacks = Callbacks ());

            void parse (std::string const & src);

            /// Parses a chunk of JSON input. This function may be called repeatedly with portions
            /// of the source data (for example, as the data is received from an external source).
            /// Once all of the data has been received, call the parser::eof() method.
            ///
            /// \param ptr A pointer to the start of the data to be parsed.
            /// \param size The number of bytes of data to be parsed.
            void parse (void const * ptr, std::size_t size);
            result_type eof ();

            ///@{

            /// \returns True if the parser has signalled an error.
            bool has_error () const noexcept { return error_ != error_code::none; }
            /// \returns The error code held by the parser.
            std::error_code last_error () const noexcept { return std::make_error_code (error_); }

            /// Records an error for this parse. The parse will stop as soon as a non-zero error
            /// code is recorded. An error may be reported at any time during the parse; all
            /// subsequent text is ignored.
            ///
            /// \param err  The json error code to be stored in the parser.
            void set_error (error_code err) noexcept {
                assert (err != error_code::none);
                error_ = err;
            }
            ///@}

            Callbacks & callbacks () noexcept { return callbacks_; }
            Callbacks const & callbacks () const noexcept { return callbacks_; }

        private:
            bool parse_impl (maybe<char> c, source & s);

            std::stack<std::unique_ptr<matcher<Callbacks>>> stack_;
            error_code error_ = error_code::none;
            Callbacks callbacks_;
        };

        template <typename Callbacks>
        inline parser<Callbacks> make_parser (Callbacks const & callbacks) {
            return parser<Callbacks> (callbacks);
        }


        class source {
        public:
            source (char const * first, char const * last) noexcept
                    : first_{first}
                    , last_{last} {}

            maybe<char> pull ();

            void push (char c) noexcept {
                assert (!lookahead_.has_value ());
                lookahead_ = c;
            }
            bool available () const noexcept { return first_ < last_ || lookahead_.has_value (); }
            char const * position () const noexcept { return first_; }
            static bool isspace (char c) noexcept {
                return c == '\x09' || c == '\x0A' || c == '\x0D' || c == '\x20';
            }


        private:
            char const * first_;
            char const * last_;
            maybe<char> lookahead_;
        };

        inline source make_source (char const * first, char const * last) noexcept {
            return {first, last};
        }


        template <typename Callbacks>
        class root_matcher;
        template <typename Callbacks>
        class trailing_ws_matcher;

        // (ctor)
        // ~~~~~~
        template <typename Callbacks>
        parser<Callbacks>::parser (Callbacks const & callbacks)
                : callbacks_{callbacks} {
            using matcher_ptr = std::unique_ptr<matcher<Callbacks>>;
            stack_.push (matcher_ptr (new trailing_ws_matcher<Callbacks> (*this)));
            stack_.push (matcher_ptr (new root_matcher<Callbacks> (*this)));
        }

        // parse_impl
        // ~~~~~~~~~~
        template <typename Callbacks>
        bool parser<Callbacks>::parse_impl (maybe<char> c, source & s) {
            auto & handler = stack_.top ();
            auto child = handler->consume (c, s);
            if (handler->is_done ()) {
                if (error_ != error_code::none) {
                    return false;
                }
                stack_.pop ();
                handler.reset ();
            }
            if (child) {
                stack_.push (std::move (child));
            }
            return true;
        }

        // parse
        // ~~~~~
        template <typename Callbacks>
        void parser<Callbacks>::parse (std::string const & str) {
            this->parse (str.data (), str.length ());
        }

        template <typename Callbacks>
        void parser<Callbacks>::parse (void const * ptr, std::size_t size) {
            if (error_ != error_code::none) {
                return;
            }
            auto orig_src = reinterpret_cast<char const *> (ptr);
            auto s = make_source (orig_src, orig_src + size);
            while (!stack_.empty () && s.available ()) {
                if (!this->parse_impl (s.pull (), s)) {
                    break;
                }
            }
        }

        template <typename T>
        struct default_return {
            static T get () { return T{}; }
        };
        template <>
        struct default_return<void> {
            static void get () { return; }
        };

        // eof
        // ~~~
        template <typename Callbacks>
        auto parser<Callbacks>::eof () -> result_type {
            auto s = make_source (nullptr, nullptr);
            while (!stack_.empty () && !has_error ()) {
                if (!this->parse_impl (nothing<char> (), s)) {
                    break;
                }
            }
            return has_error () ? default_return<result_type>::get ()
                                : this->callbacks ().result ();
        }

        /// The base class for the various state machines ("matchers") which implement the various
        /// productions of the JSON grammar.
        template <typename Callbacks>
        class matcher {
        public:
            virtual ~matcher () {}

            /// Called for each character as it is consumed from the input.
            /// \returns A matcher instance to be pushed onto the parse stack or nullptr if the same
            /// matcher object should be used to process the next character.
            virtual std::unique_ptr<matcher<Callbacks>> consume (maybe<char> ch, source & s) = 0;

            /// \returns True if this matcher has completed (and reached it's "done" state). The
            /// parser should pop this instance from the parse stack before continuing.
            bool is_done () const noexcept { return state_ & done_bit_; }

        protected:
            matcher (parser<Callbacks> & parser, int initial_state) noexcept
                    : parser_{parser}
                    , state_{initial_state} {}

            parser<Callbacks> & get_parser () const noexcept { return parser_; }

            int get_state () const noexcept { return state_; }
            void set_state (int s) noexcept { state_ = s; }

            ///@{
            /// \brief Errors

            void set_error (error_code err) noexcept {
                parser_.set_error (err);
                set_state (done_bit_);
            }
            bool has_error () const noexcept { return parser_.has_error (); }
            std::error_code last_error () const noexcept { return parser_.last_error (); }
            ///@}

            Callbacks & callbacks () noexcept { return parser_.callbacks (); }
            Callbacks const & callbacks () const noexcept { return parser_.callbacks (); }

            static constexpr std::uint8_t done_bit_ = 0x80U;

        private:
            parser<Callbacks> & parser_;
            int state_;
        };

        //*  _       _             *
        //* | |_ ___| |_____ _ _   *
        //* |  _/ _ \ / / -_) ' \  *
        //*  \__\___/_\_\___|_||_| *
        //*                        *
        /// A matcher which checks for a specific keyword such as "true", "false", or "null".
        template <typename Callbacks, typename ValueFunction = std::function<void()>>
        class token_matcher : public matcher<Callbacks> {
        public:
            token_matcher (parser<Callbacks> & parser, char const * text,
                           ValueFunction value_function) noexcept
                    : matcher<Callbacks> (parser, start_state)
                    , text_{text}
                    , value_function_{value_function} {}

            std::unique_ptr<matcher<Callbacks>> consume (maybe<char> ch, source & s) override;

        private:
            enum state {
                start_state,
                last_state,
                done_state = matcher<Callbacks>::done_bit_,
            };

            /// The keyword to be matched. The input sequence must exactly match this string or an
            /// unrecognized token error is raised. Once all of the characters are match,
            /// value_function_ is called.
            char const * text_;
            ValueFunction value_function_;
        };

        // consume
        // ~~~~~~~
        template <typename Callbacks, typename ValueFunction>
        std::unique_ptr<matcher<Callbacks>>
        token_matcher<Callbacks, ValueFunction>::consume (maybe<char> ch, source & s) {
            switch (this->get_state ()) {
            case start_state:
                if (!ch || *ch != *text_) {
                    this->set_error (error_code::unrecognized_token);
                } else {
                    ++text_;
                    if (*text_ == '\0') {
                        // We've run out of input text, so ensure that the next character isn't
                        // alpha-numeric.
                        this->set_state (last_state);
                    }
                }
                break;
            case last_state:
                if (ch) {
                    if (std::isalnum (*ch)) {
                        this->set_error (error_code::unrecognized_token);
                        return nullptr;
                    }
                    s.push (*ch);
                }
                value_function_ ();
                this->set_state (done_state);
                break;
            case done_state: assert (false); break;
            }
            return nullptr;
        }


        //*   __      _           _       _             *
        //*  / _|__ _| |___ ___  | |_ ___| |_____ _ _   *
        //* |  _/ _` | (_-</ -_) |  _/ _ \ / / -_) ' \  *
        //* |_| \__,_|_/__/\___|  \__\___/_\_\___|_||_| *
        //*                                             *
        template <typename Callbacks>
        class false_token_matcher final : public token_matcher<Callbacks> {
        public:
            false_token_matcher (parser<Callbacks> & parser)
                    : token_matcher<Callbacks> (
                          parser, "false",
                          std::bind (&Callbacks::boolean_value, &parser.callbacks (), false)) {}
        };

        //*  _                  _       _             *
        //* | |_ _ _ _  _ ___  | |_ ___| |_____ _ _   *
        //* |  _| '_| || / -_) |  _/ _ \ / / -_) ' \  *
        //*  \__|_|  \_,_\___|  \__\___/_\_\___|_||_| *
        //*                                           *
        template <typename Callbacks>
        class true_token_matcher final : public token_matcher<Callbacks> {
        public:
            true_token_matcher (parser<Callbacks> & parser)
                    : token_matcher<Callbacks> (
                          parser, "true",
                          std::bind (&Callbacks::boolean_value, &parser.callbacks (), true)) {}
        };

        //*           _ _   _       _             *
        //*  _ _ _  _| | | | |_ ___| |_____ _ _   *
        //* | ' \ || | | | |  _/ _ \ / / -_) ' \  *
        //* |_||_\_,_|_|_|  \__\___/_\_\___|_||_| *
        //*                                       *
        template <typename Callbacks>
        class null_token_matcher final : public token_matcher<Callbacks> {
        public:
            null_token_matcher (parser<Callbacks> & parser)
                    : token_matcher<Callbacks> (
                          parser, "null",
                          std::bind (&Callbacks::null_value, &parser.callbacks ())) {}
        };

        //*                 _              *
        //*  _ _ _  _ _ __ | |__  ___ _ _  *
        //* | ' \ || | '  \| '_ \/ -_) '_| *
        //* |_||_\_,_|_|_|_|_.__/\___|_|   *
        //*                                *
        // Grammar (from RFC 7159, March 2014)
        //     number = [ minus ] int [ frac ] [ exp ]
        //     decimal-point = %x2E       ; .
        //     digit1-9 = %x31-39         ; 1-9
        //     e = %x65 / %x45            ; e E
        //     exp = e [ minus / plus ] 1*DIGIT
        //     frac = decimal-point 1*DIGIT
        //     int = zero / ( digit1-9 *DIGIT )
        //     minus = %x2D               ; -
        //     plus = %x2B                ; +
        //     zero = %x30                ; 0
        template <typename Callbacks>
        class number_matcher final : public matcher<Callbacks> {
        public:
            number_matcher (parser<Callbacks> & parser) noexcept
                    : matcher<Callbacks> (parser, leading_minus_state) {}

            std::unique_ptr<matcher<Callbacks>> consume (maybe<char> ch, source & s) override;

        private:
            bool in_terminal_state () const;

            void do_leading_minus_state (char c);
            void do_integer_initial_digit_state (char c);
            void do_integer_digit_state (source & s, char c);
            void do_frac_state (char c, source & s);
            void do_frac_digit_state (char c, source & s);
            void do_exponent_sign_state (char c, source & s);
            void do_exponent_digit_state (char c, source & s);

            void complete ();
            void number_is_float ();

            void make_result ();

            enum state {
                leading_minus_state,
                integer_initial_digit_state,
                integer_digit_state,
                frac_state,
                frac_initial_digit_state,
                frac_digit_state,
                exponent_sign_state,
                exponent_initial_digit_state,
                exponent_digit_state,
                done_state = matcher<Callbacks>::done_bit_,
            };

            bool is_neg_ = false;
            bool is_integer_ = true;
            unsigned long int_acc_ = 0UL;

            struct {
                double frac_part = 0.0;
                double frac_scale = 1.0;
                double whole_part = 0.0;

                bool exp_is_negative = false;
                unsigned exponent = 0;
            } fp_acc_;
        };

        // number_is_float
        // ~~~~~~~~~~~~~~~
        template <typename Callbacks>
        void number_matcher<Callbacks>::number_is_float () {
            if (is_integer_) {
                fp_acc_.whole_part = int_acc_;
                is_integer_ = false;
            }
        }

        // in_terminal_state
        // ~~~~~~~~~~~~~~~~~
        template <typename Callbacks>
        bool number_matcher<Callbacks>::in_terminal_state () const {
            switch (this->get_state ()) {
            case integer_digit_state:
            case frac_state:
            case frac_digit_state:
            case exponent_digit_state:
            case done_state: return true;
            default: return false;
            }
        }

        // leading_minus_state
        // ~~~~~~~~~~~~~~~~~~~
        template <typename Callbacks>
        void number_matcher<Callbacks>::do_leading_minus_state (char c) {
            if (c == '-') {
                this->set_state (integer_initial_digit_state);
                is_neg_ = true;
            } else if (c >= '0' && c <= '9') {
                this->set_state (integer_initial_digit_state);
                do_integer_initial_digit_state (c);
            } else {
                // minus MUST be followed by the 'int' production.
                this->set_error (error_code::number_out_of_range);
            }
        }

        // frac_state
        // ~~~~~~~~~~
        template <typename Callbacks>
        void number_matcher<Callbacks>::do_frac_state (char c, source & s) {
            if (c == '.') {
                this->set_state (frac_initial_digit_state);
            } else if (c == 'e' || c == 'E') {
                this->set_state (exponent_sign_state);
            } else if (c >= '0' && c <= '9') {
                // digits are definitely not part of the next token so we can issue an error right
                // here.
                this->set_error (error_code::number_out_of_range);
            } else {
                // the 'frac' production is optional.
                s.push (c);
                this->complete ();
            }
        }

        // frac_digit
        // ~~~~~~~~~~
        template <typename Callbacks>
        void number_matcher<Callbacks>::do_frac_digit_state (char c, source & s) {
            assert (this->get_state () == frac_initial_digit_state ||
                    this->get_state () == frac_digit_state);
            if (c == 'e' || c == 'E') {
                this->number_is_float ();
                if (this->get_state () == frac_initial_digit_state) {
                    this->set_error (error_code::unrecognized_token);
                } else {
                    this->set_state (exponent_sign_state);
                }
            } else if (c >= '0' && c <= '9') {
                this->number_is_float ();
                fp_acc_.frac_part = fp_acc_.frac_part * 10 + (c - '0');
                fp_acc_.frac_scale *= 10;

                this->set_state (frac_digit_state);
            } else {
                if (this->get_state () == frac_initial_digit_state) {
                    this->set_error (error_code::unrecognized_token);
                } else {
                    s.push (c);
                    this->complete ();
                }
            }
        }

        // exponent_sign_state
        // ~~~~~~~~~~~~~~~~~~~
        template <typename Callbacks>
        void number_matcher<Callbacks>::do_exponent_sign_state (char c, source & s) {
            this->number_is_float ();
            this->set_state (exponent_initial_digit_state);
            switch (c) {
            case '+': fp_acc_.exp_is_negative = false; break;
            case '-': fp_acc_.exp_is_negative = true; break;
            default: this->do_exponent_digit_state (c, s); break;
            }
        }

        // complete
        // ~~~~~~~~
        template <typename Callbacks>
        void number_matcher<Callbacks>::complete () {
            this->set_state (done_state);
            this->make_result ();
        }

        // exponent_digit
        // ~~~~~~~~~~~~~~
        template <typename Callbacks>
        void number_matcher<Callbacks>::do_exponent_digit_state (char c, source & s) {
            assert (this->get_state () == exponent_digit_state ||
                    this->get_state () == exponent_initial_digit_state);
            assert (!is_integer_);
            if (c >= '0' && c <= '9') {
                fp_acc_.exponent = fp_acc_.exponent * 10U + static_cast<unsigned> (c - '0');
                this->set_state (exponent_digit_state);
            } else {
                if (this->get_state () == exponent_initial_digit_state) {
                    this->set_error (error_code::unrecognized_token);
                } else {
                    s.push (c);
                    this->complete ();
                }
            }
        }

        // do_integer_initial_digit_state
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // implements the first character of the 'int' production.
        template <typename Callbacks>
        void number_matcher<Callbacks>::do_integer_initial_digit_state (char c) {
            assert (this->get_state () == integer_initial_digit_state);
            assert (is_integer_);
            if (c == '0') {
                this->set_state (frac_state);
            } else if (c >= '1' && c <= '9') {
                assert (int_acc_ == 0);
                int_acc_ = static_cast<unsigned> (c - '0');
                this->set_state (integer_digit_state);
            } else {
                this->set_error (error_code::unrecognized_token);
            }
        }

        // integer_digit
        // ~~~~~~~~~~~~~
        template <typename Callbacks>
        void number_matcher<Callbacks>::do_integer_digit_state (source & s, char c) {
            assert (this->get_state () == integer_digit_state);
            assert (is_integer_);
            if (c == '.') {
                this->set_state (frac_initial_digit_state);
                number_is_float ();
            } else if (c == 'e' || c == 'E') {
                this->set_state (exponent_sign_state);
                number_is_float ();
            } else if (c >= '0' && c <= '9') {
                if (int_acc_ > std::numeric_limits<decltype (int_acc_)>::max () / 10U + 10U) {
                    this->set_error (error_code::number_out_of_range);
                } else {
                    int_acc_ = int_acc_ * 10U + static_cast<unsigned> (c - '0');
                }
            } else {
                s.push (c);
                this->complete ();
            }
        }

        // consume
        // ~~~~~~~
        template <typename Callbacks>
        std::unique_ptr<matcher<Callbacks>> number_matcher<Callbacks>::consume (maybe<char> ch,
                                                                                source & s) {
            if (ch) {
                char const c = *ch;
                switch (this->get_state ()) {
                case leading_minus_state: this->do_leading_minus_state (c); break;
                case integer_initial_digit_state: this->do_integer_initial_digit_state (c); break;
                case integer_digit_state: this->do_integer_digit_state (s, c); break;
                case frac_state: this->do_frac_state (c, s); break;
                case frac_initial_digit_state:
                case frac_digit_state: this->do_frac_digit_state (c, s); break;
                case exponent_sign_state: this->do_exponent_sign_state (c, s); break;
                case exponent_initial_digit_state:
                case exponent_digit_state: this->do_exponent_digit_state (c, s); break;
                case done_state: assert (false); break;
                }
            } else {
                assert (!this->has_error ());
                if (!this->in_terminal_state ()) {
                    this->set_error (error_code::expected_digits);
                }
                this->complete ();
            }
            return nullptr;
        }

        template <typename Callbacks>
        void number_matcher<Callbacks>::make_result () {
            if (this->has_error ()) {
                return;
            }
            assert (this->in_terminal_state ());

            if (is_integer_) {
                using acc_type = typename std::make_signed<decltype (int_acc_)>::type;
                using uacc_type = typename std::make_unsigned<decltype (int_acc_)>::type;
                constexpr auto max = std::numeric_limits<acc_type>::max ();
                constexpr auto min = std::numeric_limits<acc_type>::min ();
                constexpr auto umin = static_cast<uacc_type> (min);

                if (is_neg_ ? int_acc_ > umin : int_acc_ > max) {
                    this->set_error (error_code::number_out_of_range);
                    return;
                }

                long lv;
                if (is_neg_) {
                    lv = (int_acc_ == umin) ? min : -static_cast<acc_type> (int_acc_);
                } else {
                    lv = static_cast<long> (int_acc_);
                }
                this->callbacks ().integer_value (lv);
                return;
            }

            auto xf = (fp_acc_.whole_part + fp_acc_.frac_part / fp_acc_.frac_scale);
            auto exp = std::pow (10, fp_acc_.exponent);
            if (std::isinf (exp)) {
                this->set_error (error_code::number_out_of_range);
                return;
            }
            if (fp_acc_.exp_is_negative) {
                exp = 1.0 / exp;
            }

            xf *= exp;
            if (is_neg_) {
                xf = -xf;
            }

            if (std::isinf (xf) || std::isnan (xf)) {
                this->set_error (error_code::number_out_of_range);
                return;
            }
            this->callbacks ().float_value (xf);
        }


        //*     _       _            *
        //*  __| |_ _ _(_)_ _  __ _  *
        //* (_-<  _| '_| | ' \/ _` | *
        //* /__/\__|_| |_|_||_\__, | *
        //*                   |___/  *
        template <typename Callbacks>
        class string_matcher final : public matcher<Callbacks> {
        public:
            string_matcher (parser<Callbacks> & parser) noexcept
                    : matcher<Callbacks> (parser, start_state) {}

            std::unique_ptr<matcher<Callbacks>> consume (maybe<char> ch, source & s) override;

        private:
            enum state {
                start_state,
                normal_char_state,
                escape_state,
                hex1_state,
                hex2_state,
                hex3_state,
                hex4_state,

                done_state = matcher<Callbacks>::done_bit_,
            };

            static maybe<unsigned> hex_value (char32_t c, unsigned value);

            class appender {
            public:
                bool append32 (char32_t code_point);
                bool append16 (char16_t cu);
                std::string && result () { return std::move (result_); }
                bool has_high_surrogate () const noexcept { return high_surrogate_ != 0; }

            private:
                std::string result_;
                char16_t high_surrogate_ = 0;
            };

            utf8_decoder decoder_;
            appender app_;
            unsigned hex_ = 0U;
        };

        template <typename Callbacks>
        bool string_matcher<Callbacks>::appender::append32 (char32_t code_point) {
            bool ok = true;
            if (this->has_high_surrogate ()) {
                // A high surrogate followed by something other than a low surrogate.
                ok = false;
            } else {
                code_point_to_utf8<char> (code_point, std::back_inserter (result_));
            }
            return ok;
        }

        template <typename Callbacks>
        bool string_matcher<Callbacks>::appender::append16 (char16_t cu) {
            bool ok = true;
            if (is_utf16_high_surrogate (cu)) {
                if (!this->has_high_surrogate ()) {
                    high_surrogate_ = cu;
                } else {
                    // A high surrogate following another high surrogate.
                    ok = false;
                }
            } else if (is_utf16_low_surrogate (cu)) {
                if (!this->has_high_surrogate ()) {
                    // A low surrogate following by something other than a high surrogate.
                    ok = false;
                } else {
                    char16_t const surrogates[] = {high_surrogate_, cu};
                    auto first = std::begin (surrogates);
                    auto last = std::end (surrogates);
                    auto code_point = char32_t{0};
                    std::tie (first, code_point) = utf16_to_code_point (first, last, nop_swapper);
                    code_point_to_utf8 (code_point, std::back_inserter (result_));
                    high_surrogate_ = 0;
                }
            } else {
                if (this->has_high_surrogate ()) {
                    // A high surrogate followed by something other than a low surrogate.
                    ok = false;
                } else {
                    auto const code_point = static_cast<char32_t> (cu);
                    code_point_to_utf8 (code_point, std::back_inserter (result_));
                }
            }
            return ok;
        }

        template <typename Callbacks>
        maybe<unsigned> string_matcher<Callbacks>::hex_value (char32_t c, unsigned value) {
            auto digit = 0U;
            if (c >= '0' && c <= '9') {
                digit = static_cast<unsigned> (c) - '0';
            } else if (c >= 'a' && c <= 'f') {
                digit = static_cast<unsigned> (c) - 'a' + 10;
            } else if (c >= 'A' && c <= 'F') {
                digit = static_cast<unsigned> (c) - 'A' + 10;
            } else {
                return nothing<unsigned> ();
            }
            return just (value * 16 + digit);
        }

        template <typename Callbacks>
        std::unique_ptr<matcher<Callbacks>> string_matcher<Callbacks>::consume (maybe<char> ch,
                                                                                source &) {
            if (!ch) {
                this->set_error (error_code::expected_close_quote);
                return nullptr;
            }
            char32_t code_point;
            bool complete;
            std::tie (code_point, complete) = decoder_.get (static_cast<std::uint8_t> (*ch));
            if (complete) {
                switch (this->get_state ()) {
                // Matches the opening quote.
                case start_state:
                    if (code_point == '"') {
                        assert (!app_.has_high_surrogate ());
                        this->set_state (normal_char_state);
                    } else {
                        this->set_error (error_code::expected_token);
                    }
                    break;
                case normal_char_state:
                    if (code_point == '"') {
                        if (app_.has_high_surrogate ()) {
                            this->set_error (error_code::bad_unicode_code_point);
                        } else {
                            // Consume the closing quote character.
                            this->callbacks ().string_value (app_.result ());
                        }
                        this->set_state (done_state);
                    } else if (code_point == '\\') {
                        this->set_state (escape_state);
                    } else if (code_point <= 0x1F) {
                        // Control characters U+0000 through U+001F MUST be escaped.
                        this->set_error (error_code::bad_unicode_code_point);
                    } else {
                        if (!app_.append32 (code_point)) {
                            this->set_error (error_code::bad_unicode_code_point);
                        }
                    }
                    break;
                case escape_state:
                    this->set_state (normal_char_state);
                    switch (code_point) {
                    case '"': code_point = '"'; break;
                    case '\\': code_point = '\\'; break;
                    case '/': code_point = '/'; break;
                    case 'b': code_point = '\b'; break;
                    case 'f': code_point = '\f'; break;
                    case 'n': code_point = '\n'; break;
                    case 'r': code_point = '\r'; break;
                    case 't': code_point = '\t'; break;
                    case 'u': this->set_state (hex1_state); break;
                    default: this->set_error (error_code::invalid_escape_char); break;
                    }
                    if (this->get_state () == normal_char_state) {
                        assert (code_point < 0x80);
                        if (!app_.append32 (code_point)) {
                            this->set_error (error_code::invalid_escape_char);
                        }
                    }
                    break;
                case hex1_state: hex_ = 0; PSTORE_FALLTHROUGH;
                case hex2_state: PSTORE_FALLTHROUGH;
                case hex3_state:
                    if (maybe<unsigned> j = hex_value (code_point, hex_)) {
                        {
                            auto s = this->get_state ();
                            switch (s) {
                            case hex1_state: s = hex2_state; break;
                            case hex2_state: s = hex3_state; break;
                            case hex3_state: s = hex4_state; break;
                            default: assert (false); break;
                            }
                            this->set_state (s);
                        }
                        hex_ = *j;
                    } else {
                        this->set_error (error_code::invalid_escape_char);
                    }
                    break;
                case hex4_state:
                    if (maybe<unsigned> j = hex_value (code_point, hex_)) {
                        this->set_state (normal_char_state);
                        hex_ = *j;
                        assert (hex_ <= std::numeric_limits<std::uint16_t>::max ());
                        if (!app_.append16 (static_cast<char16_t> (hex_))) {
                            this->set_error (error_code::bad_unicode_code_point);
                        }
                    } else {
                        this->set_error (error_code::bad_unicode_code_point);
                    }
                    break;
                case done_state: assert (false); break;
                }
            }
            return nullptr;
        }

        //*                          *
        //*  __ _ _ _ _ _ __ _ _  _  *
        //* / _` | '_| '_/ _` | || | *
        //* \__,_|_| |_| \__,_|\_, | *
        //*                    |__/  *
        template <typename Callbacks>
        class array_matcher final : public matcher<Callbacks> {
        public:
            array_matcher (parser<Callbacks> & parser) noexcept
                    : matcher<Callbacks> (parser, start_state) {}

            std::unique_ptr<matcher<Callbacks>> consume (maybe<char> ch, source & s) override;

        private:
            enum state {
                start_state,
                object_state,
                comma_state,
                done_state = matcher<Callbacks>::done_bit_,
            };
            bool is_first_ = true; // FIXME: use an additional state instead.
        };

        // consume
        // ~~~~~~~
        template <typename Callbacks>
        std::unique_ptr<matcher<Callbacks>> array_matcher<Callbacks>::consume (maybe<char> ch,
                                                                               source & s) {
            if (!ch) {
                this->set_error (error_code::expected_array_member);
                return nullptr;
            }
            using matcher_ptr = std::unique_ptr<matcher<Callbacks>>;
            matcher_ptr result;
            char c = *ch;
            switch (this->get_state ()) {
            case start_state:
                assert (c == '[');
                this->callbacks ().begin_array ();
                this->set_state (object_state);
                break;
            case object_state:
                if (source::isspace (c)) {
                    // just consume whitespace before an object (or close bracket)
                } else if (c == ']' && is_first_) {
                    this->callbacks ().end_array ();
                    this->set_state (done_state);
                } else {
                    is_first_ = false;
                    s.push (c);
                    this->set_state (comma_state);
                    result = matcher_ptr (new root_matcher<Callbacks> (this->get_parser ()));
                }
                break;
            case comma_state:
                if (source::isspace (c)) {
                    // just consume whitespace before a comma.
                } else if (c == ',') {
                    this->set_state (object_state);
                } else if (c == ']') {
                    this->callbacks ().end_array ();
                    this->set_state (done_state);
                } else {
                    this->set_error (error_code::expected_array_member);
                }
                break;
            case done_state: assert (false); break;
            }
            return result;
        }

        //*      _     _        _    *
        //*  ___| |__ (_)___ __| |_  *
        //* / _ \ '_ \| / -_) _|  _| *
        //* \___/_.__// \___\__|\__| *
        //*         |__/             *
        template <typename Callbacks>
        class object_matcher final : public matcher<Callbacks> {
        public:
            object_matcher (parser<Callbacks> & parser) noexcept
                    : matcher<Callbacks> (parser, start_state) {}

            std::unique_ptr<matcher<Callbacks>> consume (maybe<char> ch, source & s) override;

        private:
            enum state {
                start_state,
                first_key_state,
                key_state,
                colon_state,
                value_state,
                comma_state,
                done_state = matcher<Callbacks>::done_bit_,
            };
        };

        // consume
        // ~~~~~~~
        template <typename Callbacks>
        std::unique_ptr<matcher<Callbacks>> object_matcher<Callbacks>::consume (maybe<char> ch,
                                                                                source & s) {
            using matcher_ptr = std::unique_ptr<matcher<Callbacks>>;
            if (this->get_state () == done_state) {
                assert (this->last_error () != std::make_error_code (error_code::none));
                return nullptr;
            }
            if (!ch) {
                this->set_error (error_code::expected_object_member);
                return nullptr;
            }
            char c = *ch;
            switch (this->get_state ()) {
            case start_state:
                assert (c == '{');
                this->set_state (first_key_state);
                this->callbacks ().begin_object ();
                break;
            case first_key_state:
                if (c == '}') {
                    this->callbacks ().end_object ();
                    this->set_state (done_state);
                    break;
                }
                PSTORE_FALLTHROUGH;
            case key_state:
                s.push (c);
                this->set_state (colon_state);
                return matcher_ptr (new root_matcher<Callbacks> (this->get_parser (),
                                                                 true /*only string allowed*/));
            case colon_state:
                if (source::isspace (c)) {
                    // just consume whitespace before the colon.
                } else if (c == ':') {
                    this->set_state (value_state);
                } else {
                    this->set_error (error_code::expected_colon);
                }
                break;
            case value_state:
                s.push (c);
                this->set_state (comma_state);
                return matcher_ptr (new root_matcher<Callbacks> (this->get_parser ()));
            case comma_state:
                if (source::isspace (c)) {
                    // just consume whitespace before the comma.
                } else if (c == ',') {
                    this->set_state (key_state);
                } else if (c == '}') {
                    this->callbacks ().end_object ();
                    this->set_state (done_state);
                } else {
                    this->set_error (error_code::expected_object_member);
                }
                break;
            case done_state: assert (false); break;
            }
            return nullptr;
        }

        template <typename Callbacks>
        class root_matcher final : public matcher<Callbacks> {
        public:
            root_matcher (parser<Callbacks> & parser, bool only_string = false) noexcept
                    : matcher<Callbacks> (parser, start_state)
                    , only_string_{only_string} {}

            std::unique_ptr<matcher<Callbacks>> consume (maybe<char> ch, source & s) override;

        private:
            enum state {
                start_state,
                new_token_state,
                done_state = matcher<Callbacks>::done_bit_,
            };
            bool const only_string_;
        };

        template <typename Callbacks>
        std::unique_ptr<matcher<Callbacks>> root_matcher<Callbacks>::consume (maybe<char> ch,
                                                                              source & s) {
            // static constexpr matcher_deleter nop_deleter (false);
            using matcher_ptr = std::unique_ptr<matcher<Callbacks>>;
            switch (this->get_state ()) {
            case start_state:
                if (!ch) {
                    this->set_error (error_code::expected_token);
                    return nullptr;
                }
                if (source::isspace (*ch)) {
                    // just consume leading whitespace
                    return nullptr;
                }
                this->set_state (new_token_state);
                PSTORE_FALLTHROUGH;
            case new_token_state: {
                s.push (*ch);
                if (only_string_ && *ch != '"') {
                    this->set_error (error_code::expected_string);
                    // Don't return here in order to allow the switch default to produce a different
                    // error code for a bad token.
                }
                this->set_state (done_state);
                switch (*ch) {
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9': return matcher_ptr (new number_matcher<Callbacks> (this->get_parser ()));
                case '"': return matcher_ptr (new string_matcher<Callbacks> (this->get_parser ()));
                case 't':
                    return matcher_ptr (new true_token_matcher<Callbacks> (this->get_parser ()));
                case 'f':
                    return matcher_ptr (new false_token_matcher<Callbacks> (this->get_parser ()));
                case 'n':
                    return matcher_ptr (new null_token_matcher<Callbacks> (this->get_parser ()));

                case '[': return matcher_ptr (new array_matcher<Callbacks> (this->get_parser ()));
                case '{': return matcher_ptr (new object_matcher<Callbacks> (this->get_parser ()));
                default: this->set_error (error_code::expected_token); break;
                }
            } break;
            case done_state: assert (false); break;
            }
            return nullptr;
        }

        //*  _            _ _ _                        *
        //* | |_ _ _ __ _(_) (_)_ _  __ _  __ __ _____ *
        //* |  _| '_/ _` | | | | ' \/ _` | \ V  V (_-< *
        //*  \__|_| \__,_|_|_|_|_||_\__, |  \_/\_//__/ *
        //*                         |___/              *
        /// \brief Consumes whitespace characters until EOF is signalled.
        /// The parser class pushes an instance of this class as the first matcher on the parse
        /// stack. This ensures that the sole top-level JSON object is followed by whitespace only
        /// and there are no spurious characters after that object.
        template <typename Callbacks>
        class trailing_ws_matcher final : public matcher<Callbacks> {
        public:
            explicit trailing_ws_matcher (parser<Callbacks> & parser) noexcept
                    : matcher<Callbacks> (parser, start_state) {}

            std::unique_ptr<matcher<Callbacks>> consume (maybe<char> ch, source & s) override;

        private:
            enum state {
                start_state,
                done_state = matcher<Callbacks>::done_bit_,
            };
        };

        template <typename Callbacks>
        std::unique_ptr<matcher<Callbacks>> trailing_ws_matcher<Callbacks>::consume (maybe<char> ch,
                                                                                     source &) {
            switch (this->get_state ()) {
            case start_state:
                if (!ch) {
                    this->set_state (done_state);
                } else {
                    if (!source::isspace (*ch)) {
                        this->set_error (error_code::unexpected_extra_input);
                    }
                }
                break;
            case done_state: assert (false); break;
            }
            return nullptr;
        }

    } // namespace json
} // namespace pstore

#endif // PSTORE_JSON_HPP
// eof: include/pstore_json/json.hpp
