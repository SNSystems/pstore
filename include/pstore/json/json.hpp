//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
//===- include/pstore/json/json.hpp ---------------------------------------===//
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

#include <cctype>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <stack>
#include <system_error>
#include <type_traits>

#include "pstore/json/json_error.hpp"
#include "pstore/json/utf.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/maybe.hpp"
#include "pstore/support/portab.hpp"

namespace pstore {
    namespace json {

        /// \brief JSON parser implementation details.
        namespace details {
            class source;

            template <typename Callbacks>
            class matcher;

            template <typename Callbacks>
            struct singleton_storage;

            /// deleter is intended for use as a unique_ptr<> Deleter. It enables unique_ptr<> to be
            /// used with a mixture of heap-allocated and placement-new-allocated objects.
            template <typename T>
            struct deleter {
                /// \param d True if the managed object should be deleted; false, if it only the
                /// detructor should be called.
                deleter (bool d = true) noexcept
                        : delete_{d} {}
                void operator() (T * const p) const noexcept {
                    if (delete_) {
                        delete p;
                    } else {
                        // this instance of T is in unowned memory: call the destructor but don't
                        // try to free the storage.
                        if (p) {
                            p->~T ();
                        }
                    }
                }

            private:
                bool delete_;
            };
        } // namespace details

        // clang-format off
        /// \tparam Callbacks  Should be a type containing the following members:
        ///     Signature | Description
        ///     ----------|------------
        ///     result_type | The type returned by the Callbacks::result() member function. This will be the type returned by parser<>::eof(). Should be default-constructible.
        ///     void string_value (std::string const & s) | Called when a JSON string has been parsed.
        ///     void integer_value (long v) | Called when an integer value has been parsed.
        ///     void float_value (double v) | Called when a floating-point value has been parsed.
        ///     void boolean_value (bool v) | Called when a boolean value has been parsed.
        ///     void null_value () | Called when a null value has been parsed.
        ///     void begin_array () | Called to notify the start of an array. Subsequent event notifications are for members of this array until a matching call to Callbacks::end_array().
        ///     void end_array () | Called indicate that an array has been completely parsed. This will always follow an earlier call to begin_array().
        ///     void begin_object () | Called to notify the start of an object. Subsequent event notifications are for members of this object until a matching call to Callbacks::end_object().
        ///     void end_object () | Called to indicate that an object has been completely parsed. This will always follow an earlier call to begin_object().
        ///     result_type result () const | Returns the result of the parse. If the parse was successful, this function is called by parser<>::elf() which will return its result.
        // clang-format on
        template <typename Callbacks>
        class parser {
            friend class details::matcher<Callbacks>;

        public:
            using result_type = typename Callbacks::result_type;

            parser (Callbacks callbacks = Callbacks ());

            ///@{
            /// Parses a chunk of JSON input. This function may be called repeatedly with portions
            /// of the source data (for example, as the data is received from an external source).
            /// Once all of the data has been received, call the parser::eof() method.

            /// A convenience function which is equivalent to calling:
            ///     parse (src.data(), src.length())
            /// \param src The data to be parsed.
            void parse (std::string const & src) { this->parse (gsl::make_span (src)); }

            /// \param src The data to be parsed.
            void parse (char const * src) {
                parse (gsl::span<char const> (src, std::strlen (src)));
            }

            /// Parses a chunk of JSON input. This function may be called repeatedly with portions
            /// of the source data (for example, as the data is received from an external source).
            /// Once all of the data has been received, call the parser::eof() method.
            ///
            /// \param span The span of bytes to be parsed.
            template <typename SpanType>
            void parse (SpanType const & span);
            ///@}

            /// Informs the parser that the complete input stream has been passed by calls to
            /// parser<>::parse().
            ///
            /// \returns If the parse completes successfully, Callbacks::result()
            /// is called and its result returned. If the parse failed, then a default-constructed
            /// instance of result_type is returned.
            result_type eof ();

            ///@{

            /// \returns True if the parser has signalled an error.
            bool has_error () const noexcept { return error_ != error_code::none; }
            /// \returns The error code held by the parser.
            std::error_code last_error () const noexcept { return std::make_error_code (error_); }

            ///@{
            Callbacks & callbacks () noexcept { return callbacks_; }
            Callbacks const & callbacks () const noexcept { return callbacks_; }
            ///@}

        private:
            using matcher = details::matcher<Callbacks>;
            using pointer = std::unique_ptr<matcher, details::deleter<matcher>>;

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

            bool parse_impl (maybe<char> c, details::source & s);

            pointer make_root_matcher (bool only_string = false);

            template <typename Matcher, typename... Args>
            pointer make_terminal_matcher (Args &&... args);

            /// Preallocated storage for "singleton" matcher. These are the matchers, such as
            /// numbers of strings, which are "terminal" and can't have child objects.
            std::unique_ptr<details::singleton_storage<Callbacks>> singletons_;
            /// The parse stack.
            std::stack<pointer> stack_;
            error_code error_ = error_code::none;
            Callbacks callbacks_;
        };

        template <typename Callbacks>
        inline parser<Callbacks> make_parser (Callbacks const & callbacks) {
            return parser<Callbacks> (callbacks);
        }

        namespace details {
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
                bool available () const noexcept {
                    return first_ < last_ || lookahead_.has_value ();
                }
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

            /// The base class for the various state machines ("matchers") which implement the
            /// various productions of the JSON grammar.
            template <typename Callbacks>
            class matcher {
            public:
                using pointer = std::unique_ptr<matcher, deleter<matcher>>;

                virtual ~matcher () = default;

                /// Called for each character as it is consumed from the input.
                ///
                /// \param parser The owning parser instance.
                /// \param ch If true, the character to be consumed. A value of nothing indicates
                /// end-of-file. \returns A matcher instance to be pushed onto the parse stack or
                /// nullptr if the same matcher object should be used to process the next character.
                virtual pointer consume (parser<Callbacks> & parser, maybe<char> ch,
                                         source & s) = 0;

                /// \returns True if this matcher has completed (and reached it's "done" state). The
                /// parser should pop this instance from the parse stack before continuing.
                bool is_done () const noexcept { return state_ == done; }

            protected:
                explicit matcher (int initial_state) noexcept
                        : state_{initial_state} {}

                int get_state () const noexcept { return state_; }
                void set_state (int s) noexcept { state_ = s; }

                ///@{
                /// \brief Errors

                void set_error (parser<Callbacks> & parser, error_code err) noexcept {
                    parser.set_error (err);
                    set_state (done);
                }
                ///@}

                pointer make_root_matcher (parser<Callbacks> & parser, bool only_string = false) {
                    return parser.make_root_matcher (only_string);
                }

                template <typename Matcher, typename... Args>
                pointer make_terminal_matcher (parser<Callbacks> & parser, Args &&... args) {
                    return parser.template make_terminal_matcher<Matcher, Args...> (
                        std::forward<Args> (args)...);
                }

                /// The value to be used for the "done" state in the each of the matcher state
                /// machines.
                static constexpr std::uint8_t done = 0xFFU;

            private:
                int state_;
            };

            //*  _       _             *
            //* | |_ ___| |_____ _ _   *
            //* |  _/ _ \ / / -_) ' \  *
            //*  \__\___/_\_\___|_||_| *
            //*                        *
            /// A matcher which checks for a specific keyword such as "true", "false", or "null".
            /// \tparam Callbacks  The parser callback structure.
            /// \tparam DoneFunction  A function matching the signature void(parser<Callbacks>&)
            /// that will be called when the token is successfully matcher.
            template <typename Callbacks, typename DoneFunction>
            class token_matcher : public matcher<Callbacks> {
            public:
                explicit token_matcher (char const * text,
                                        DoneFunction done_fn = DoneFunction ()) noexcept
                        : matcher<Callbacks> (start_state)
                        , text_{text}
                        , done_fn_{std::move (done_fn)} {}

                typename matcher<Callbacks>::pointer consume (parser<Callbacks> & parser,
                                                              maybe<char> ch, source & s) override;

            private:
                enum state {
                    start_state,
                    last_state,
                    done_state = matcher<Callbacks>::done,
                };

                /// The keyword to be matched. The input sequence must exactly match this string or
                /// an unrecognized token error is raised. Once all of the characters are matched,
                /// complete() is called.
                char const * text_;

                /// This function is called once the complete token text has been matched.
                DoneFunction done_fn_;
            };


            // consume
            // ~~~~~~~
            template <typename Callbacks, typename DoneFunction>
            typename matcher<Callbacks>::pointer
            token_matcher<Callbacks, DoneFunction>::consume (parser<Callbacks> & parser,
                                                             maybe<char> ch, source & s) {
                switch (this->get_state ()) {
                case start_state:
                    if (!ch || *ch != *text_) {
                        this->set_error (parser, error_code::unrecognized_token);
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
                            this->set_error (parser, error_code::unrecognized_token);
                            return nullptr;
                        }
                        s.push (*ch);
                    }
                    done_fn_ (parser);
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

            struct false_complete {
                template <typename Callbacks>
                void operator() (parser<Callbacks> & parser) const {
                    parser.callbacks ().boolean_value (false);
                }
            };

            template <typename Callbacks>
            using false_token_matcher = token_matcher<Callbacks, false_complete>;

            //*  _                  _       _             *
            //* | |_ _ _ _  _ ___  | |_ ___| |_____ _ _   *
            //* |  _| '_| || / -_) |  _/ _ \ / / -_) ' \  *
            //*  \__|_|  \_,_\___|  \__\___/_\_\___|_||_| *
            //*                                           *

            struct true_complete {
                template <typename Callbacks>
                void operator() (parser<Callbacks> & parser) const {
                    parser.callbacks ().boolean_value (true);
                }
            };

            template <typename Callbacks>
            using true_token_matcher = token_matcher<Callbacks, true_complete>;

            //*           _ _   _       _             *
            //*  _ _ _  _| | | | |_ ___| |_____ _ _   *
            //* | ' \ || | | | |  _/ _ \ / / -_) ' \  *
            //* |_||_\_,_|_|_|  \__\___/_\_\___|_||_| *
            //*                                       *

            struct null_complete {
                template <typename Callbacks>
                void operator() (parser<Callbacks> & parser) const {
                    parser.callbacks ().null_value ();
                }
            };

            template <typename Callbacks>
            using null_token_matcher = token_matcher<Callbacks, null_complete>;

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
                number_matcher () noexcept
                        : matcher<Callbacks> (leading_minus_state) {}

                typename matcher<Callbacks>::pointer consume (parser<Callbacks> & parser,
                                                              maybe<char> ch, source & s) override;

            private:
                bool in_terminal_state () const;

                void do_leading_minus_state (parser<Callbacks> & parser, char c);
                void do_integer_initial_digit_state (parser<Callbacks> & parser, char c);
                void do_integer_digit_state (parser<Callbacks> & parser, source & s, char c);
                void do_frac_state (parser<Callbacks> & parser, char c, source & s);
                void do_frac_digit_state (parser<Callbacks> & parser, char c, source & s);
                void do_exponent_sign_state (parser<Callbacks> & parser, char c, source & s);
                void do_exponent_digit_state (parser<Callbacks> & parser, char c, source & s);

                void complete (parser<Callbacks> & parser);
                void number_is_float ();

                void make_result (parser<Callbacks> & parser);

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
                    done_state = matcher<Callbacks>::done,
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
            void number_matcher<Callbacks>::do_leading_minus_state (parser<Callbacks> & parser,
                                                                    char c) {
                if (c == '-') {
                    this->set_state (integer_initial_digit_state);
                    is_neg_ = true;
                } else if (c >= '0' && c <= '9') {
                    this->set_state (integer_initial_digit_state);
                    do_integer_initial_digit_state (parser, c);
                } else {
                    // minus MUST be followed by the 'int' production.
                    this->set_error (parser, error_code::number_out_of_range);
                }
            }

            // frac_state
            // ~~~~~~~~~~
            template <typename Callbacks>
            void number_matcher<Callbacks>::do_frac_state (parser<Callbacks> & parser, char c,
                                                           source & s) {
                if (c == '.') {
                    this->set_state (frac_initial_digit_state);
                } else if (c == 'e' || c == 'E') {
                    this->set_state (exponent_sign_state);
                } else if (c >= '0' && c <= '9') {
                    // digits are definitely not part of the next token so we can issue an error
                    // right here.
                    this->set_error (parser, error_code::number_out_of_range);
                } else {
                    // the 'frac' production is optional.
                    s.push (c);
                    this->complete (parser);
                }
            }

            // frac_digit
            // ~~~~~~~~~~
            template <typename Callbacks>
            void number_matcher<Callbacks>::do_frac_digit_state (parser<Callbacks> & parser, char c,
                                                                 source & s) {
                assert (this->get_state () == frac_initial_digit_state ||
                        this->get_state () == frac_digit_state);
                if (c == 'e' || c == 'E') {
                    this->number_is_float ();
                    if (this->get_state () == frac_initial_digit_state) {
                        this->set_error (parser, error_code::unrecognized_token);
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
                        this->set_error (parser, error_code::unrecognized_token);
                    } else {
                        s.push (c);
                        this->complete (parser);
                    }
                }
            }

            // exponent_sign_state
            // ~~~~~~~~~~~~~~~~~~~
            template <typename Callbacks>
            void number_matcher<Callbacks>::do_exponent_sign_state (parser<Callbacks> & parser,
                                                                    char c, source & s) {
                this->number_is_float ();
                this->set_state (exponent_initial_digit_state);
                switch (c) {
                case '+': fp_acc_.exp_is_negative = false; break;
                case '-': fp_acc_.exp_is_negative = true; break;
                default: this->do_exponent_digit_state (parser, c, s); break;
                }
            }

            // complete
            // ~~~~~~~~
            template <typename Callbacks>
            void number_matcher<Callbacks>::complete (parser<Callbacks> & parser) {
                this->set_state (done_state);
                this->make_result (parser);
            }

            // exponent_digit
            // ~~~~~~~~~~~~~~
            template <typename Callbacks>
            void number_matcher<Callbacks>::do_exponent_digit_state (parser<Callbacks> & parser,
                                                                     char c, source & s) {
                assert (this->get_state () == exponent_digit_state ||
                        this->get_state () == exponent_initial_digit_state);
                assert (!is_integer_);
                if (c >= '0' && c <= '9') {
                    fp_acc_.exponent = fp_acc_.exponent * 10U + static_cast<unsigned> (c - '0');
                    this->set_state (exponent_digit_state);
                } else {
                    if (this->get_state () == exponent_initial_digit_state) {
                        this->set_error (parser, error_code::unrecognized_token);
                    } else {
                        s.push (c);
                        this->complete (parser);
                    }
                }
            }

            // do_integer_initial_digit_state
            // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            // implements the first character of the 'int' production.
            template <typename Callbacks>
            void
            number_matcher<Callbacks>::do_integer_initial_digit_state (parser<Callbacks> & parser,
                                                                       char c) {
                assert (this->get_state () == integer_initial_digit_state);
                assert (is_integer_);
                if (c == '0') {
                    this->set_state (frac_state);
                } else if (c >= '1' && c <= '9') {
                    assert (int_acc_ == 0);
                    int_acc_ = static_cast<unsigned> (c - '0');
                    this->set_state (integer_digit_state);
                } else {
                    this->set_error (parser, error_code::unrecognized_token);
                }
            }

            // integer_digit
            // ~~~~~~~~~~~~~
            template <typename Callbacks>
            void number_matcher<Callbacks>::do_integer_digit_state (parser<Callbacks> & parser,
                                                                    source & s, char c) {
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
                        this->set_error (parser, error_code::number_out_of_range);
                    } else {
                        int_acc_ = int_acc_ * 10U + static_cast<unsigned> (c - '0');
                    }
                } else {
                    s.push (c);
                    this->complete (parser);
                }
            }

            // consume
            // ~~~~~~~
            template <typename Callbacks>
            typename matcher<Callbacks>::pointer
            number_matcher<Callbacks>::consume (parser<Callbacks> & parser, maybe<char> ch,
                                                source & s) {
                if (ch) {
                    char const c = *ch;
                    switch (this->get_state ()) {
                    case leading_minus_state: this->do_leading_minus_state (parser, c); break;
                    case integer_initial_digit_state:
                        this->do_integer_initial_digit_state (parser, c);
                        break;
                    case integer_digit_state: this->do_integer_digit_state (parser, s, c); break;
                    case frac_state: this->do_frac_state (parser, c, s); break;
                    case frac_initial_digit_state:
                    case frac_digit_state: this->do_frac_digit_state (parser, c, s); break;
                    case exponent_sign_state: this->do_exponent_sign_state (parser, c, s); break;
                    case exponent_initial_digit_state:
                    case exponent_digit_state: this->do_exponent_digit_state (parser, c, s); break;
                    case done_state: assert (false); break;
                    }
                } else {
                    assert (!parser.has_error ());
                    if (!this->in_terminal_state ()) {
                        this->set_error (parser, error_code::expected_digits);
                    }
                    this->complete (parser);
                }
                return nullptr;
            }

            template <typename Callbacks>
            void number_matcher<Callbacks>::make_result (parser<Callbacks> & parser) {
                if (parser.has_error ()) {
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
                        this->set_error (parser, error_code::number_out_of_range);
                        return;
                    }

                    long lv;
                    if (is_neg_) {
                        lv = (int_acc_ == umin) ? min : -static_cast<acc_type> (int_acc_);
                    } else {
                        lv = static_cast<long> (int_acc_);
                    }
                    parser.callbacks ().integer_value (lv);
                    return;
                }

                auto xf = (fp_acc_.whole_part + fp_acc_.frac_part / fp_acc_.frac_scale);
                auto exp = std::pow (10, fp_acc_.exponent);
                if (std::isinf (exp)) {
                    this->set_error (parser, error_code::number_out_of_range);
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
                    this->set_error (parser, error_code::number_out_of_range);
                    return;
                }
                parser.callbacks ().float_value (xf);
            }


            //*     _       _            *
            //*  __| |_ _ _(_)_ _  __ _  *
            //* (_-<  _| '_| | ' \/ _` | *
            //* /__/\__|_| |_|_||_\__, | *
            //*                   |___/  *
            template <typename Callbacks>
            class string_matcher final : public matcher<Callbacks> {
            public:
                string_matcher () noexcept
                        : matcher<Callbacks> (start_state) {}

                typename matcher<Callbacks>::pointer consume (parser<Callbacks> & parser,
                                                              maybe<char> ch, source & s) override;

            private:
                enum state {
                    start_state,
                    normal_char_state,
                    escape_state,
                    hex1_state,
                    hex2_state,
                    hex3_state,
                    hex4_state,
                    done_state = matcher<Callbacks>::done,
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
                        std::tie (first, code_point) =
                            utf16_to_code_point (first, last, nop_swapper);
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
            typename matcher<Callbacks>::pointer
            string_matcher<Callbacks>::consume (parser<Callbacks> & parser, maybe<char> ch,
                                                source &) {
                if (!ch) {
                    this->set_error (parser, error_code::expected_close_quote);
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
                            this->set_error (parser, error_code::expected_token);
                        }
                        break;
                    case normal_char_state:
                        if (code_point == '"') {
                            if (app_.has_high_surrogate ()) {
                                this->set_error (parser, error_code::bad_unicode_code_point);
                            } else {
                                // Consume the closing quote character.
                                parser.callbacks ().string_value (app_.result ());
                            }
                            this->set_state (done_state);
                        } else if (code_point == '\\') {
                            this->set_state (escape_state);
                        } else if (code_point <= 0x1F) {
                            // Control characters U+0000 through U+001F MUST be escaped.
                            this->set_error (parser, error_code::bad_unicode_code_point);
                        } else {
                            if (!app_.append32 (code_point)) {
                                this->set_error (parser, error_code::bad_unicode_code_point);
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
                        default: this->set_error (parser, error_code::invalid_escape_char); break;
                        }
                        if (this->get_state () == normal_char_state) {
                            assert (code_point < 0x80);
                            if (!app_.append32 (code_point)) {
                                this->set_error (parser, error_code::invalid_escape_char);
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
                            this->set_error (parser, error_code::invalid_escape_char);
                        }
                        break;
                    case hex4_state:
                        if (maybe<unsigned> j = hex_value (code_point, hex_)) {
                            this->set_state (normal_char_state);
                            hex_ = *j;
                            assert (hex_ <= std::numeric_limits<std::uint16_t>::max ());
                            if (!app_.append16 (static_cast<char16_t> (hex_))) {
                                this->set_error (parser, error_code::bad_unicode_code_point);
                            }
                        } else {
                            this->set_error (parser, error_code::bad_unicode_code_point);
                        }
                        break;
                    case done_state: assert (false); break;
                    }
                }
                return nullptr;
            }


            template <typename Callbacks>
            class root_matcher;

            //*                          *
            //*  __ _ _ _ _ _ __ _ _  _  *
            //* / _` | '_| '_/ _` | || | *
            //* \__,_|_| |_| \__,_|\_, | *
            //*                    |__/  *
            template <typename Callbacks>
            class array_matcher final : public matcher<Callbacks> {
            public:
                array_matcher () noexcept
                        : matcher<Callbacks> (start_state) {}

                typename matcher<Callbacks>::pointer consume (parser<Callbacks> & parser,
                                                              maybe<char> ch, source & s) override;

            private:
                enum state {
                    start_state,
                    object_state,
                    comma_state,
                    done_state = matcher<Callbacks>::done,
                };
                bool is_first_ = true; // FIXME: use an additional state instead.
            };

            // consume
            // ~~~~~~~
            template <typename Callbacks>
            typename matcher<Callbacks>::pointer
            array_matcher<Callbacks>::consume (parser<Callbacks> & parser, maybe<char> ch,
                                               source & s) {
                if (!ch) {
                    this->set_error (parser, error_code::expected_array_member);
                    return nullptr;
                }
                char c = *ch;
                switch (this->get_state ()) {
                case start_state:
                    assert (c == '[');
                    parser.callbacks ().begin_array ();
                    this->set_state (object_state);
                    break;
                case object_state:
                    if (source::isspace (c)) {
                        // just consume whitespace before an object (or close bracket)
                    } else if (c == ']' && is_first_) {
                        parser.callbacks ().end_array ();
                        this->set_state (done_state);
                    } else {
                        is_first_ = false;
                        s.push (c);
                        this->set_state (comma_state);
                        return this->make_root_matcher (parser);
                    }
                    break;
                case comma_state:
                    if (source::isspace (c)) {
                        // just consume whitespace before a comma.
                    } else if (c == ',') {
                        this->set_state (object_state);
                    } else if (c == ']') {
                        parser.callbacks ().end_array ();
                        this->set_state (done_state);
                    } else {
                        this->set_error (parser, error_code::expected_array_member);
                    }
                    break;
                case done_state: assert (false); break;
                }
                return nullptr;
            }

            //*      _     _        _    *
            //*  ___| |__ (_)___ __| |_  *
            //* / _ \ '_ \| / -_) _|  _| *
            //* \___/_.__// \___\__|\__| *
            //*         |__/             *
            template <typename Callbacks>
            class object_matcher final : public matcher<Callbacks> {
            public:
                object_matcher () noexcept
                        : matcher<Callbacks> (start_state) {}

                typename matcher<Callbacks>::pointer consume (parser<Callbacks> & parser,
                                                              maybe<char> ch, source & s) override;

            private:
                enum state {
                    start_state,
                    first_key_state,
                    key_state,
                    colon_state,
                    value_state,
                    comma_state,
                    done_state = matcher<Callbacks>::done,
                };
            };

            // consume
            // ~~~~~~~
            template <typename Callbacks>
            typename matcher<Callbacks>::pointer
            object_matcher<Callbacks>::consume (parser<Callbacks> & parser, maybe<char> ch,
                                                source & s) {
                if (this->get_state () == done_state) {
                    assert (parser.last_error () != std::make_error_code (error_code::none));
                    return nullptr;
                }
                if (!ch) {
                    this->set_error (parser, error_code::expected_object_member);
                    return nullptr;
                }
                char c = *ch;
                switch (this->get_state ()) {
                case start_state:
                    assert (c == '{');
                    this->set_state (first_key_state);
                    parser.callbacks ().begin_object ();
                    break;
                case first_key_state:
                    if (c == '}') {
                        parser.callbacks ().end_object ();
                        this->set_state (done_state);
                        break;
                    }
                    PSTORE_FALLTHROUGH;
                case key_state:
                    s.push (c);
                    this->set_state (colon_state);
                    return this->make_root_matcher (parser, true /*only string allowed*/);
                case colon_state:
                    if (source::isspace (c)) {
                        // just consume whitespace before the colon.
                    } else if (c == ':') {
                        this->set_state (value_state);
                    } else {
                        this->set_error (parser, error_code::expected_colon);
                    }
                    break;
                case value_state:
                    s.push (c);
                    this->set_state (comma_state);
                    return this->make_root_matcher (parser);
                case comma_state:
                    if (source::isspace (c)) {
                        // just consume whitespace before the comma.
                    } else if (c == ',') {
                        this->set_state (key_state);
                    } else if (c == '}') {
                        parser.callbacks ().end_object ();
                        this->set_state (done_state);
                    } else {
                        this->set_error (parser, error_code::expected_object_member);
                    }
                    break;
                case done_state: assert (false); break;
                }
                return nullptr;
            }

            template <typename Callbacks>
            class root_matcher final : public matcher<Callbacks> {
            public:
                root_matcher (bool only_string = false) noexcept
                        : matcher<Callbacks> (start_state)
                        , only_string_{only_string} {}

                typename matcher<Callbacks>::pointer consume (parser<Callbacks> & parser,
                                                              maybe<char> ch, source & s) override;

            private:
                enum state {
                    start_state,
                    new_token_state,
                    done_state = matcher<Callbacks>::done,
                };
                bool const only_string_;
            };

            template <typename Callbacks>
            typename matcher<Callbacks>::pointer
            root_matcher<Callbacks>::consume (parser<Callbacks> & parser, maybe<char> ch,
                                              source & s) {
                using pointer = typename matcher<Callbacks>::pointer;
                switch (this->get_state ()) {
                case start_state:
                    if (!ch) {
                        this->set_error (parser, error_code::expected_token);
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
                        this->set_error (parser, error_code::expected_string);
                        // Don't return here in order to allow the switch default to produce a
                        // different error code for a bad token.
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
                    case '9':
                        return this->template make_terminal_matcher<number_matcher<Callbacks>> (
                            parser);
                    case '"':
                        return this->template make_terminal_matcher<string_matcher<Callbacks>> (
                            parser);
                    case 't':
                        return this->template make_terminal_matcher<true_token_matcher<Callbacks>> (
                            parser, "true");
                    case 'f':
                        return this
                            ->template make_terminal_matcher<false_token_matcher<Callbacks>> (
                                parser, "false");
                    case 'n':
                        return this->template make_terminal_matcher<null_token_matcher<Callbacks>> (
                            parser, "null");

                        // TODO: limit the maximum nesting to some large number of objects (avoids a
                        // DOS attack on the listening socket).
                    case '[': return pointer (new array_matcher<Callbacks> ());
                    case '{': return pointer (new object_matcher<Callbacks> ());
                    default: this->set_error (parser, error_code::expected_token); break;
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
            /// stack. This ensures that the sole top-level JSON object is followed by whitespace
            /// only and there are no spurious characters after that object.
            template <typename Callbacks>
            class trailing_ws_matcher final : public matcher<Callbacks> {
            public:
                trailing_ws_matcher () noexcept
                        : matcher<Callbacks> (start_state) {}
                typename matcher<Callbacks>::pointer consume (parser<Callbacks> & parser,
                                                              maybe<char> ch, source & s) override;

            private:
                enum state {
                    start_state,
                    done_state = matcher<Callbacks>::done,
                };
            };

            template <typename Callbacks>
            typename matcher<Callbacks>::pointer
            trailing_ws_matcher<Callbacks>::consume (parser<Callbacks> & parser, maybe<char> ch,
                                                     source &) {
                switch (this->get_state ()) {
                case start_state:
                    if (!ch) {
                        this->set_state (done_state);
                    } else {
                        if (!source::isspace (*ch)) {
                            this->set_error (parser, error_code::unexpected_extra_input);
                        }
                    }
                    break;
                case done_state: assert (false); break;
                }
                return nullptr;
            }



            /// In C++14 std::max() is a constexpr function so we could use it in these templates,
            /// but our code needs to be compatible with C++11.
            template <typename T, T V1, T V2>
            struct max : std::integral_constant<T, (V1 < V2) ? V2 : V1> {};

            // max_sizeof
            // ~~~~~~~~~~
            template <typename... T>
            struct max_sizeof;
            template <>
            struct max_sizeof<> : std::integral_constant<std::size_t, 1U> {};
            template <typename Head, typename... Tail>
            struct max_sizeof<Head, Tail...>
                    : std::integral_constant<std::size_t, max<std::size_t, sizeof (Head),
                                                              max_sizeof<Tail...>::value>::value> {
            };

            // max_alignment
            // ~~~~~~~~~~~~~
            template <typename... Types>
            struct max_alignment;
            template <>
            struct max_alignment<> : std::integral_constant<std::size_t, 1U> {};
            template <typename Head, typename... Tail>
            struct max_alignment<Head, Tail...>
                    : std::integral_constant<
                          std::size_t,
                          max<std::size_t, alignof (Head), max_alignment<Tail...>::value>::value> {
            };

            // characteristics
            // ~~~~~~~~~~~~~~~
            /// Given a list of types, find the size of the largest type and the alignment of the
            /// most aligned type.
            template <typename... T>
            struct characteristics {
                static constexpr std::size_t size = max_sizeof<T...>::value;
                static constexpr std::size_t align = max_alignment<T...>::value;
            };

            //*     _           _     _                 _                          *
            //*  __(_)_ _  __ _| |___| |_ ___ _ _    __| |_ ___ _ _ __ _ __ _ ___  *
            //* (_-< | ' \/ _` | / -_)  _/ _ \ ' \  (_-<  _/ _ \ '_/ _` / _` / -_) *
            //* /__/_|_||_\__, |_\___|\__\___/_||_| /__/\__\___/_| \__,_\__, \___| *
            //*           |___/                                         |___/      *
            template <typename Callbacks>
            struct singleton_storage {
                template <typename T>
                struct storage {
                    using type = typename std::aligned_storage<sizeof (T), alignof (T)>::type;
                };

                typename storage<trailing_ws_matcher<Callbacks>>::type trailing_ws;
                typename storage<root_matcher<Callbacks>>::type root;

                using matcher_characteristics =
                    characteristics<number_matcher<Callbacks>, string_matcher<Callbacks>,
                                    true_token_matcher<Callbacks>, false_token_matcher<Callbacks>,
                                    null_token_matcher<Callbacks>>;

                typename std::aligned_storage<matcher_characteristics::size,
                                              matcher_characteristics::align>::type terminal;
            };

            /// Returns a default-initialized instance of type T.
            template <typename T>
            struct default_return {
                static T get () { return T{}; }
            };
            template <>
            struct default_return<void> {
                static void get () { return; }
            };
        } // namespace details


        // (ctor)
        // ~~~~~~
        template <typename Callbacks>
        parser<Callbacks>::parser (Callbacks callbacks)
                : singletons_{new details::singleton_storage<Callbacks>}
                , callbacks_{std::move (callbacks)} {
            using pointer = typename matcher::pointer;
            using trailing_ws_matcher = details::trailing_ws_matcher<Callbacks>;

            stack_.push (pointer (new (&singletons_->trailing_ws) trailing_ws_matcher (), false));
            stack_.push (this->make_root_matcher ());
        }

        // make_root_matcher
        // ~~~~~~~~~~~~~~~~~
        template <typename Callbacks>
        auto parser<Callbacks>::make_root_matcher (bool only_string_allowed) -> pointer {
            using root_matcher = details::root_matcher<Callbacks>;
            return pointer (new (&singletons_->root) root_matcher (only_string_allowed), false);
        }

        // make_terminal_matcher
        // ~~~~~~~~~~~~~~~~~~~~~
        template <typename Callbacks>
        template <typename Matcher, typename... Args>
        auto parser<Callbacks>::make_terminal_matcher (Args &&... args) -> pointer {
            static_assert (sizeof (Matcher) <= sizeof (decltype (singletons_->terminal)),
                           "terminal storage is not large enough for Matcher type");
            static_assert (alignof (Matcher) <= alignof (decltype (singletons_->terminal)),
                           "terminal storage is not sufficiently aligned for Matcher type");

            return pointer (new (&singletons_->terminal) Matcher (std::forward<Args> (args)...),
                            false);
        }

        // parse_impl
        // ~~~~~~~~~~
        template <typename Callbacks>
        bool parser<Callbacks>::parse_impl (maybe<char> c, details::source & s) {
            auto & handler = stack_.top ();
            auto child = handler->consume (*this, c, s);
            if (handler->is_done ()) {
                if (error_ != error_code::none) {
                    return false;
                }
                // release the topmost matcher object.
                stack_.pop ();
            }
            if (child) {
                stack_.push (std::move (child));
            }
            return true;
        }

        // parse
        // ~~~~~
        template <typename Callbacks>
        template <typename SpanType>
        void parser<Callbacks>::parse (SpanType const & span) {
            static_assert (
                std::is_same<typename std::remove_cv<typename SpanType::element_type>::type,
                             char>::value,
                "span element type must be char");
            if (error_ != error_code::none) {
                return;
            }
            auto orig_src = span.data ();
            auto s = details::make_source (orig_src, orig_src + span.size ());
            while (!stack_.empty () && s.available ()) {
                if (!this->parse_impl (s.pull (), s)) {
                    break;
                }
            }
        }

        // eof
        // ~~~
        template <typename Callbacks>
        auto parser<Callbacks>::eof () -> result_type {
            auto s = details::make_source (nullptr, nullptr);
            while (!stack_.empty () && !has_error ()) {
                if (!this->parse_impl (nothing<char> (), s)) {
                    break;
                }
            }
            return has_error () ? details::default_return<result_type>::get ()
                                : this->callbacks ().result ();
        }

    } // namespace json
} // namespace pstore

#endif // PSTORE_JSON_HPP
// eof: include/pstore/json/json.hpp
