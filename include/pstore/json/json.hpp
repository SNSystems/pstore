//===- include/pstore/json/json.hpp -----------------------*- mode: C++ -*-===//
//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_JSON_JSON_HPP
#define PSTORE_JSON_JSON_HPP

#include <cctype>
#include <cmath>
#include <cstring>
#include <ostream>
#include <stack>
#include <tuple>

#include "pstore/json/json_error.hpp"
#include "pstore/support/max.hpp"
#include "pstore/support/utf.hpp"

namespace pstore {
    namespace json {

        /// \brief JSON parser implementation details.
        namespace details {

            template <typename Callbacks>
            class matcher;
            template <typename Callbacks>
            class root_matcher;
            template <typename Callbacks>
            class whitespace_matcher;

            template <typename Callbacks>
            struct singleton_storage;

            /// deleter is intended for use as a unique_ptr<> Deleter. It enables unique_ptr<> to be
            /// used with a mixture of heap-allocated and placement-new-allocated objects.
            template <typename T>
            class deleter {
            public:
                /// \param d True if the managed object should be deleted; false, if it only the
                /// detructor should be called.
                constexpr explicit deleter (bool const d = true) noexcept
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

        } // end namespace details

        struct coord {
            constexpr coord (unsigned const x, unsigned const y) noexcept
                    : column{x}
                    , row{y} {}
            bool operator== (coord const & rhs) const noexcept {
                return column == rhs.column && row == rhs.row;
            }
            bool operator!= (coord const & rhs) const noexcept { return !operator== (rhs); }

            unsigned column;
            unsigned row;
        };

        inline std::ostream & operator<< (std::ostream & os, coord const & c) {
            return os << '(' << c.row << ':' << c.column << ')';
        }

        enum class extensions : unsigned {
            none = 0U,
            bash_comments = 1U << 0U,
            single_line_comments = 1U << 1U,
            multi_line_comments = 1U << 2U,
            array_trailing_comma = 1U << 3U,
            object_trailing_comma = 1U << 4U,
            all = ~none,
        };

        inline constexpr extensions operator| (extensions a, extensions b) noexcept {
            return static_cast<extensions> (static_cast<unsigned> (a) | static_cast<unsigned> (b));
        }


        // clang-format off
        /// \tparam Callbacks  Should be a type containing the following members:
        ///     Signature | Description
        ///     ----------|------------
        ///     result_type | The type returned by the Callbacks::result() member function. This will be the type returned by parser<>::eof(). Should be default-constructible.
        ///     std::error_code string_value (std::string const & s) | Called when a JSON string has been parsed.
        ///     std::error_code int64_value (std::int64_t v) | Called when an integer value has been parsed.
        ///     std::error_code uint64_value (std::uint64_t v) | Called when an unsigned integer value has been parsed.
        ///     std::error_code double_value (double v) | Called when a floating-point value has been parsed.
        ///     std::error_code boolean_value (bool v) | Called when a boolean value has been parsed.
        ///     std::error_code null_value () | Called when a null value has been parsed.
        ///     std::error_code begin_array () | Called to notify the start of an array. Subsequent event notifications are for members of this array until a matching call to Callbacks::end_array().
        ///     std::error_code end_array () | Called indicate that an array has been completely parsed. This will always follow an earlier call to begin_array().
        ///     std::error_code begin_object () | Called to notify the start of an object. Subsequent event notifications are for members of this object until a matching call to Callbacks::end_object().
        ///     std::error_code key (std::string const & s) | Called when an object key string has been parsed.
        ///     std::error_code end_object () | Called to indicate that an object has been completely parsed. This will always follow an earlier call to begin_object().
        ///     result_type result () const | Returns the result of the parse. If the parse was successful, this function is called by parser<>::eof() which will return its result.
        // clang-format on
        //-MARK:parser
        template <typename Callbacks>
        class parser {
            friend class details::matcher<Callbacks>;
            friend class details::root_matcher<Callbacks>;
            friend class details::whitespace_matcher<Callbacks>;

        public:
            using result_type = typename Callbacks::result_type;

            explicit parser (Callbacks callbacks = Callbacks{},
                             extensions extensions = extensions::none);

            ///@{
            /// Parses a chunk of JSON input. This function may be called repeatedly with portions
            /// of the source data (for example, as the data is received from an external source).
            /// Once all of the data has been received, call the parser::eof() method.

            /// A convenience function which is equivalent to calling:
            ///     input (gsl::make_span (src))
            /// \param src The data to be parsed.
            parser & input (std::string const & src) { return this->input (gsl::make_span (src)); }

            /// A convenience function which is equivalent to calling:
            ///     input (gsl::make_span (src, std::strlen (src)))
            /// \param src The data to be parsed.
            parser & input (gsl::czstring const src) {
                std::size_t const len = std::strlen (src);
                using index_type = gsl::span<char>::index_type;
                PSTORE_ASSERT (len <= static_cast<std::make_unsigned_t<index_type>> (
                                          std::numeric_limits<index_type>::max ()));
                return this->input (gsl::make_span (src, static_cast<index_type> (len)));
            }

            /// \param span The span of UTF-8 code units to be parsed.
            template <typename SpanType>
            parser & input (SpanType const & span);

            /// \param first The element in the half-open range of UTF-8 code-units to be parsed.
            /// \param last The end of the range of UTF-8 code-units to be parsed.
            template <typename InputIterator>
            parser & input (InputIterator first, InputIterator last);

            ///@}

            /// Informs the parser that the complete input stream has been passed by calls to
            /// parser<>::input().
            ///
            /// \returns If the parse completes successfully, Callbacks::result()
            /// is called and its result returned. If the parse failed, then a default-constructed
            /// instance of result_type is returned.
            result_type eof ();

            ///@{

            /// \returns True if the parser has signalled an error.
            constexpr bool has_error () const noexcept { return static_cast<bool> (error_); }
            /// \returns The error code held by the parser.
            constexpr std::error_code const & last_error () const noexcept { return error_; }

            ///@{
            constexpr Callbacks & callbacks () noexcept { return callbacks_; }
            constexpr Callbacks const & callbacks () const noexcept { return callbacks_; }
            ///@}

            /// \param flag  A selection of bits from the parser_extensions enum.
            /// \returns True if any of the extensions given by \p flag are enabled by the parser.
            constexpr bool extension_enabled (extensions const flag) const noexcept {
                using ut = std::underlying_type_t<extensions>;
                return (static_cast<ut> (extensions_) & static_cast<ut> (flag)) != 0U;
            }

            /// Returns the parser's position in the input text.
            constexpr coord coordinate () const noexcept { return coordinate_; }

        private:
            using matcher = details::matcher<Callbacks>;
            using pointer = std::unique_ptr<matcher, details::deleter<matcher>>;

            ///@{
            /// \brief Managing the column and row number (the "coordinate").

            /// Increments the column number.
            void advance_column () noexcept { ++coordinate_.column; }

            /// Increments the row number and resets the column.
            void advance_row () noexcept {
                // The column number is set to 0. This is because the outer parse loop automatically
                // advances the column number for each character consumed. This happens after the
                // row is advanced by a matcher's consume() function.
                coordinate_.column = 0U;
                ++coordinate_.row;
            }

            /// Resets the column count but does not affect the row number.
            void reset_column () noexcept { coordinate_.column = 0U; }
            ///@}

            /// Records an error for this parse. The parse will stop as soon as a non-zero error
            /// code is recorded. An error may be reported at any time during the parse; all
            /// subsequent text is ignored.
            ///
            /// \param err  The json error code to be stored in the parser.
            bool set_error (std::error_code const err) noexcept {
                PSTORE_ASSERT (!error_ || err);
                error_ = err;
                return this->has_error ();
            }
            ///@}

            pointer make_root_matcher (bool object_key = false);
            pointer make_whitespace_matcher ();

            template <typename Matcher, typename... Args>
            pointer make_terminal_matcher (Args &&... args);

            void const * get_terminal_storage () const noexcept;

            /// Preallocated storage for "singleton" matcher. These are the matchers, such as
            /// numbers of strings, which are "terminal" and can't have child objects.
            std::unique_ptr<details::singleton_storage<Callbacks>> singletons_{
                new details::singleton_storage<Callbacks>};
            /// The maximum depth to which we allow the parse stack to grow. This value should be
            /// sufficient for any reasonable input: its intention is to prevent bogus (attack)
            /// inputs from taking the parser down.
            static constexpr std::size_t max_stack_depth_ = 200;
            /// The parse stack.
            std::stack<pointer> stack_;
            std::error_code error_;
            Callbacks callbacks_;
            extensions const extensions_;

            /// Each instance of the string matcher uses this string object to record its output.
            /// This avoids having to create a new instance each time we scan a string.
            std::string string_;

            /// The column and row number of the parse within the input stream.
            coord coordinate_{1U, 1U};
        };

        template <typename Callbacks>
        inline parser<Callbacks> make_parser (Callbacks const & callbacks,
                                              extensions const extensions = extensions::none) {
            return parser<Callbacks>{callbacks, extensions};
        }

        namespace details {

            enum char_set : char {
                cr = '\x0D',
                hash = '#',
                lf = '\x0A',
                slash = '/',
                space = '\x20',
                star = '*',
                tab = '\x09',
            };
            constexpr bool isspace (char const c) noexcept {
                return c == char_set::tab || c == char_set::lf || c == char_set::cr ||
                       c == char_set::space;
            }

            /// The base class for the various state machines ("matchers") which implement the
            /// various productions of the JSON grammar.
            //-MARK:matcher
            template <typename Callbacks>
            class matcher {
            public:
                using pointer = std::unique_ptr<matcher, deleter<matcher>>;

                matcher (matcher const & ) = delete;
                matcher (matcher && ) noexcept = delete;

                virtual ~matcher () = default;

                matcher & operator= (matcher const & ) = delete;
                matcher & operator= (matcher && ) noexcept = delete;

                /// Called for each character as it is consumed from the input.
                ///
                /// \param parser The owning parser instance.
                /// \param ch If true, the character to be consumed. A value of nothing indicates
                /// end-of-file.
                /// \returns A pair consisting of a matcher pointer and a boolean. If non-null, the
                /// matcher is pushed onto the parse stack; if null the same matcher object is used
                /// to process the next character. The boolean value is false if the same character
                /// must be passed to the next consume() call; true indicates that the character was
                /// correctly matched by this consume() call.
                virtual std::pair<pointer, bool> consume (parser<Callbacks> & parser,
                                                          maybe<char> ch) = 0;

                /// \returns True if this matcher has completed (and reached it's "done" state). The
                /// parser will pop this instance from the parse stack before continuing.
                bool is_done () const noexcept { return state_ == done; }

            protected:
                explicit constexpr matcher (int const initial_state) noexcept
                        : state_{initial_state} {}

                constexpr int get_state () const noexcept { return state_; }
                void set_state (int const s) noexcept { state_ = s; }

                ///@{
                /// \brief Errors

                /// \returns True if the parser is in an error state.
                bool set_error (parser<Callbacks> & parser, std::error_code const & err) noexcept {
                    bool const has_error = parser.set_error (err);
                    if (has_error) {
                        set_state (done);
                    }
                    return has_error;
                }
                ///@}

                pointer make_root_matcher (parser<Callbacks> & parser, bool object_key = false) {
                    return parser.make_root_matcher (object_key);
                }
                pointer make_whitespace_matcher (parser<Callbacks> & parser) {
                    return parser.make_whitespace_matcher ();
                }

                template <typename Matcher, typename... Args>
                pointer make_terminal_matcher (parser<Callbacks> & parser, Args &&... args) {
                    PSTORE_ASSERT (this != parser.get_terminal_storage ());
                    return parser.template make_terminal_matcher<Matcher, Args...> (
                        std::forward<Args> (args)...);
                }

                /// The value to be used for the "done" state in the each of the matcher state
                /// machines.
                static constexpr auto done = std::uint8_t{1};

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
            ///   that will be called when the token is successfully matched.
            //-MARK:token matcher
            template <typename Callbacks, typename DoneFunction>
            class token_matcher : public matcher<Callbacks> {
            public:
                /// \param text  The string to be matched.
                /// \param done_fn  The function called when the source string is matched.
                explicit token_matcher (gsl::czstring const text,
                                        DoneFunction done_fn = DoneFunction ()) noexcept
                        : matcher<Callbacks> (start_state)
                        , text_{text}
                        , done_{std::move (done_fn)} {}

                std::pair<typename matcher<Callbacks>::pointer, bool>
                consume (parser<Callbacks> & parser, maybe<char> ch) override;

            private:
                enum state {
                    done_state = matcher<Callbacks>::done,
                    start_state,
                    last_state,
                };

                /// The keyword to be matched. The input sequence must exactly match this string or
                /// an unrecognized token error is raised. Once all of the characters are matched,
                /// complete() is called.
                gsl::czstring text_;

                /// This function is called once the complete token text has been matched.
                DoneFunction done_;
            };

            template <typename Callbacks, typename DoneFunction>
            std::pair<typename matcher<Callbacks>::pointer, bool>
            token_matcher<Callbacks, DoneFunction>::consume (parser<Callbacks> & parser,
                                                             maybe<char> ch) {
                bool match = true;
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
                        if (std::isalnum (*ch) != 0) {
                            this->set_error (parser, error_code::unrecognized_token);
                            return {nullptr, true};
                        }
                        match = false;
                    }
                    this->set_error (parser, done_ (parser));
                    this->set_state (done_state);
                    break;
                case done_state: PSTORE_ASSERT (false); break;
                }
                return {nullptr, match};
            }

            //*   __      _           _       _             *
            //*  / _|__ _| |___ ___  | |_ ___| |_____ _ _   *
            //* |  _/ _` | (_-</ -_) |  _/ _ \ / / -_) ' \  *
            //* |_| \__,_|_/__/\___|  \__\___/_\_\___|_||_| *
            //*                                             *

            //-MARK:false token
            struct false_complete {
                template <typename Callbacks>
                std::error_code operator() (parser<Callbacks> & parser) const {
                    return parser.callbacks ().boolean_value (false);
                }
            };

            template <typename Callbacks>
            using false_token_matcher = token_matcher<Callbacks, false_complete>;

            //*  _                  _       _             *
            //* | |_ _ _ _  _ ___  | |_ ___| |_____ _ _   *
            //* |  _| '_| || / -_) |  _/ _ \ / / -_) ' \  *
            //*  \__|_|  \_,_\___|  \__\___/_\_\___|_||_| *
            //*                                           *

            //-MARK:true token
            struct true_complete {
                template <typename Callbacks>
                std::error_code operator() (parser<Callbacks> & parser) const {
                    return parser.callbacks ().boolean_value (true);
                }
            };

            template <typename Callbacks>
            using true_token_matcher = token_matcher<Callbacks, true_complete>;

            //*           _ _   _       _             *
            //*  _ _ _  _| | | | |_ ___| |_____ _ _   *
            //* | ' \ || | | | |  _/ _ \ / / -_) ' \  *
            //* |_||_\_,_|_|_|  \__\___/_\_\___|_||_| *
            //*                                       *

            //-MARK:null token
            struct null_complete {
                template <typename Callbacks>
                std::error_code operator() (parser<Callbacks> & parser) const {
                    return parser.callbacks ().null_value ();
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
            //-MARK:number
            template <typename Callbacks>
            class number_matcher final : public matcher<Callbacks> {
            public:
                number_matcher () noexcept
                        : matcher<Callbacks> (leading_minus_state) {}

                std::pair<typename matcher<Callbacks>::pointer, bool>
                consume (parser<Callbacks> & parser, maybe<char> ch) override;

            private:
                bool in_terminal_state () const;

                bool do_leading_minus_state (parser<Callbacks> & parser, char c);
                /// Implements the first character of the 'int' production.
                bool do_integer_initial_digit_state (parser<Callbacks> & parser, char c);
                bool do_integer_digit_state (parser<Callbacks> & parser, char c);
                bool do_frac_state (parser<Callbacks> & parser, char c);
                bool do_frac_digit_state (parser<Callbacks> & parser, char c);
                bool do_exponent_sign_state (parser<Callbacks> & parser, char c);
                bool do_exponent_digit_state (parser<Callbacks> & parser, char c);

                void complete (parser<Callbacks> & parser);
                void number_is_float ();

                void make_result (parser<Callbacks> & parser);

                enum state {
                    done_state = matcher<Callbacks>::done,
                    leading_minus_state,
                    integer_initial_digit_state,
                    integer_digit_state,
                    frac_state,
                    frac_initial_digit_state,
                    frac_digit_state,
                    exponent_sign_state,
                    exponent_initial_digit_state,
                    exponent_digit_state,
                };

                bool is_neg_ = false;
                bool is_integer_ = true;
                std::uint64_t int_acc_ = 0;

                struct {
                    double frac_part = 0.0;
                    double frac_scale = 1.0;
                    double whole_part = 0.0;

                    bool exp_is_negative = false;
                    unsigned exponent = 0;
                } fp_acc_;
            };

            // number is float
            // ~~~~~~~~~~~~~~~
            template <typename Callbacks>
            void number_matcher<Callbacks>::number_is_float () {
                if (is_integer_) {
                    fp_acc_.whole_part = static_cast<double> (int_acc_);
                    is_integer_ = false;
                }
            }

            // in terminal state
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

            // leading minus state
            // ~~~~~~~~~~~~~~~~~~~
            template <typename Callbacks>
            bool number_matcher<Callbacks>::do_leading_minus_state (parser<Callbacks> & parser,
                                                                    char c) {
                bool match = true;
                if (c == '-') {
                    this->set_state (integer_initial_digit_state);
                    is_neg_ = true;
                } else if (c >= '0' && c <= '9') {
                    this->set_state (integer_initial_digit_state);
                    match = do_integer_initial_digit_state (parser, c);
                } else {
                    // minus MUST be followed by the 'int' production.
                    this->set_error (parser, error_code::number_out_of_range);
                }
                return match;
            }

            // frac state
            // ~~~~~~~~~~
            template <typename Callbacks>
            bool number_matcher<Callbacks>::do_frac_state (parser<Callbacks> & parser,
                                                           char const c) {
                bool match = true;
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
                    match = false;
                    this->complete (parser);
                }
                return match;
            }

            // frac digit
            // ~~~~~~~~~~
            template <typename Callbacks>
            bool number_matcher<Callbacks>::do_frac_digit_state (parser<Callbacks> & parser,
                                                                 char const c) {
                PSTORE_ASSERT (this->get_state () == frac_initial_digit_state ||
                               this->get_state () == frac_digit_state);

                bool match = true;
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
                        match = false;
                        this->complete (parser);
                    }
                }
                return match;
            }

            // exponent sign state
            // ~~~~~~~~~~~~~~~~~~~
            template <typename Callbacks>
            bool number_matcher<Callbacks>::do_exponent_sign_state (parser<Callbacks> & parser,
                                                                    char c) {
                bool match = true;
                this->number_is_float ();
                this->set_state (exponent_initial_digit_state);
                switch (c) {
                case '+': fp_acc_.exp_is_negative = false; break;
                case '-': fp_acc_.exp_is_negative = true; break;
                default: match = this->do_exponent_digit_state (parser, c); break;
                }
                return match;
            }

            // complete
            // ~~~~~~~~
            template <typename Callbacks>
            void number_matcher<Callbacks>::complete (parser<Callbacks> & parser) {
                this->set_state (done_state);
                this->make_result (parser);
            }

            // exponent digit
            // ~~~~~~~~~~~~~~
            template <typename Callbacks>
            bool number_matcher<Callbacks>::do_exponent_digit_state (parser<Callbacks> & parser,
                                                                     char const c) {
                PSTORE_ASSERT (this->get_state () == exponent_digit_state ||
                               this->get_state () == exponent_initial_digit_state);
                PSTORE_ASSERT (!is_integer_);

                bool match = true;
                if (c >= '0' && c <= '9') {
                    fp_acc_.exponent = fp_acc_.exponent * 10U + static_cast<unsigned> (c - '0');
                    this->set_state (exponent_digit_state);
                } else {
                    if (this->get_state () == exponent_initial_digit_state) {
                        this->set_error (parser, error_code::unrecognized_token);
                    } else {
                        match = false;
                        this->complete (parser);
                    }
                }
                return match;
            }

            // do integer initial digit state
            // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            template <typename Callbacks>
            bool
            number_matcher<Callbacks>::do_integer_initial_digit_state (parser<Callbacks> & parser,
                                                                       char const c) {
                PSTORE_ASSERT (this->get_state () == integer_initial_digit_state);
                PSTORE_ASSERT (is_integer_);
                if (c == '0') {
                    this->set_state (frac_state);
                } else if (c >= '1' && c <= '9') {
                    PSTORE_ASSERT (int_acc_ == 0);
                    int_acc_ = static_cast<unsigned> (c - '0');
                    this->set_state (integer_digit_state);
                } else {
                    this->set_error (parser, error_code::unrecognized_token);
                }
                return true;
            }

            // do integer digit state
            // ~~~~~~~~~~~~~~~~~~~~~~
            template <typename Callbacks>
            bool number_matcher<Callbacks>::do_integer_digit_state (parser<Callbacks> & parser,
                                                                    char const c) {
                PSTORE_ASSERT (this->get_state () == integer_digit_state);
                PSTORE_ASSERT (is_integer_);

                bool match = true;
                if (c == '.') {
                    this->set_state (frac_initial_digit_state);
                    number_is_float ();
                } else if (c == 'e' || c == 'E') {
                    this->set_state (exponent_sign_state);
                    number_is_float ();
                } else if (c >= '0' && c <= '9') {
                    std::uint64_t const new_acc = int_acc_ * 10U + static_cast<unsigned> (c - '0');
                    if (new_acc < int_acc_) { // Did this overflow?
                        this->set_error (parser, error_code::number_out_of_range);
                    } else {
                        int_acc_ = new_acc;
                    }
                } else {
                    match = false;
                    this->complete (parser);
                }
                return match;
            }

            // consume
            // ~~~~~~~
            template <typename Callbacks>
            std::pair<typename matcher<Callbacks>::pointer, bool>
            number_matcher<Callbacks>::consume (parser<Callbacks> & parser, maybe<char> ch) {
                bool match = true;
                if (ch) {
                    char const c = *ch;
                    switch (this->get_state ()) {
                    case leading_minus_state:
                        match = this->do_leading_minus_state (parser, c);
                        break;
                    case integer_initial_digit_state:
                        match = this->do_integer_initial_digit_state (parser, c);
                        break;
                    case integer_digit_state:
                        match = this->do_integer_digit_state (parser, c);
                        break;
                    case frac_state: match = this->do_frac_state (parser, c); break;
                    case frac_initial_digit_state:
                    case frac_digit_state: match = this->do_frac_digit_state (parser, c); break;
                    case exponent_sign_state:
                        match = this->do_exponent_sign_state (parser, c);
                        break;
                    case exponent_initial_digit_state:
                    case exponent_digit_state:
                        match = this->do_exponent_digit_state (parser, c);
                        break;
                    case done_state: PSTORE_ASSERT (false); break;
                    }
                } else {
                    PSTORE_ASSERT (!parser.has_error ());
                    if (!this->in_terminal_state ()) {
                        this->set_error (parser, error_code::expected_digits);
                    }
                    this->complete (parser);
                }
                return {nullptr, match};
            }

            // make result
            // ~~~~~~~~~~~
            template <typename Callbacks>
            void number_matcher<Callbacks>::make_result (parser<Callbacks> & parser) {
                if (parser.has_error ()) {
                    return;
                }
                PSTORE_ASSERT (this->in_terminal_state ());

                if (is_integer_) {
                    constexpr auto min = std::numeric_limits<std::int64_t>::min ();
                    constexpr auto umin = static_cast<std::uint64_t> (min);

                    if (is_neg_) {
                        if (int_acc_ > umin) {
                            this->set_error (parser, error_code::number_out_of_range);
                            return;
                        }

                        this->set_error (
                            parser,
                            parser.callbacks ().int64_value (
                                (int_acc_ == umin) ? min : -static_cast<std::int64_t> (int_acc_)));
                        return;
                    }
                    this->set_error (parser, parser.callbacks ().uint64_value (int_acc_));
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
                this->set_error (parser, parser.callbacks ().double_value (xf));
            }


            //*     _       _            *
            //*  __| |_ _ _(_)_ _  __ _  *
            //* (_-<  _| '_| | ' \/ _` | *
            //* /__/\__|_| |_|_||_\__, | *
            //*                   |___/  *
            //-MARK:string
            template <typename Callbacks>
            class string_matcher final : public matcher<Callbacks> {
            public:
                explicit string_matcher (gsl::not_null<std::string *> const str,
                                         bool object_key) noexcept
                        : matcher<Callbacks> (start_state)
                        , object_key_{object_key}
                        , app_{str} {
                    str->clear ();
                }

                std::pair<typename matcher<Callbacks>::pointer, bool>
                consume (parser<Callbacks> & parser, maybe<char> ch) override;

            private:
                enum state {
                    done_state = matcher<Callbacks>::done,
                    start_state,
                    normal_char_state,
                    escape_state,
                    hex1_state,
                    hex2_state,
                    hex3_state,
                    hex4_state,
                };

                class appender {
                public:
                    explicit appender (gsl::not_null<std::string *> const result) noexcept
                            : result_{result} {}
                    bool append32 (char32_t code_point);
                    bool append16 (char16_t cu);
                    gsl::not_null<std::string *> result () { return result_; }
                    bool has_high_surrogate () const noexcept { return high_surrogate_ != 0; }

                private:
                    gsl::not_null<std::string *> const result_;
                    char16_t high_surrogate_ = 0;
                };

                std::tuple<state, std::error_code> consume_normal_state (parser<Callbacks> & parser,
                                                                         char32_t code_point,
                                                                         appender & app);

                static maybe<unsigned> hex_value (char32_t c, unsigned value);

                static maybe<std::tuple<unsigned, state>>
                consume_hex_state (unsigned hex, enum state state, char32_t code_point);

                static std::tuple<state, error_code> consume_escape_state (char32_t code_point,
                                                                           appender & app);
                bool object_key_;
                utf::utf8_decoder decoder_;
                appender app_;
                unsigned hex_ = 0U;
            };

            // append32
            // ~~~~~~~~
            template <typename Callbacks>
            bool string_matcher<Callbacks>::appender::append32 (char32_t const code_point) {
                bool ok = true;
                if (this->has_high_surrogate ()) {
                    // A high surrogate followed by something other than a low surrogate.
                    ok = false;
                } else {
                    utf::code_point_to_utf8<char> (code_point, std::back_inserter (*result_));
                }
                return ok;
            }

            // append16
            // ~~~~~~~~
            template <typename Callbacks>
            bool string_matcher<Callbacks>::appender::append16 (char16_t const cu) {
                bool ok = true;
                if (utf::is_utf16_high_surrogate (cu)) {
                    if (!this->has_high_surrogate ()) {
                        high_surrogate_ = cu;
                    } else {
                        // A high surrogate following another high surrogate.
                        ok = false;
                    }
                } else if (utf::is_utf16_low_surrogate (cu)) {
                    if (!this->has_high_surrogate ()) {
                        // A low surrogate following by something other than a high surrogate.
                        ok = false;
                    } else {
                        char16_t const surrogates[] = {high_surrogate_, cu};
                        auto first = std::begin (surrogates);
                        auto const last = std::end (surrogates);
                        auto code_point = char32_t{0};
                        std::tie (first, code_point) =
                            utf::utf16_to_code_point (first, last, utf::nop_swapper);
                        utf::code_point_to_utf8 (code_point, std::back_inserter (*result_));
                        high_surrogate_ = 0;
                    }
                } else {
                    if (this->has_high_surrogate ()) {
                        // A high surrogate followed by something other than a low surrogate.
                        ok = false;
                    } else {
                        auto const code_point = static_cast<char32_t> (cu);
                        utf::code_point_to_utf8 (code_point, std::back_inserter (*result_));
                    }
                }
                return ok;
            }

            // consume normal state
            // ~~~~~~~~~~~~~~~~~~~~
            template <typename Callbacks>
            auto string_matcher<Callbacks>::consume_normal_state (parser<Callbacks> & parser,
                                                                  char32_t code_point,
                                                                  appender & app)
                -> std::tuple<state, std::error_code> {
                state next_state = normal_char_state;
                std::error_code error;

                if (code_point == '"') {
                    if (app.has_high_surrogate ()) {
                        error = error_code::bad_unicode_code_point;
                    } else {
                        // Consume the closing quote character.
                        if (object_key_) {
                            error = parser.callbacks ().key (*app.result ());
                        } else {
                            error = parser.callbacks ().string_value (*app.result ());
                        }
                    }
                    next_state = done_state;
                } else if (code_point == '\\') {
                    next_state = escape_state;
                } else if (code_point <= 0x1F) {
                    // Control characters U+0000 through U+001F MUST be escaped.
                    error = error_code::bad_unicode_code_point;
                } else {
                    if (!app.append32 (code_point)) {
                        error = error_code::bad_unicode_code_point;
                    }
                }

                return std::make_tuple (next_state, error);
            }

            // hex value [static]
            // ~~~~~~~~~
            template <typename Callbacks>
            maybe<unsigned> string_matcher<Callbacks>::hex_value (char32_t const c,
                                                                  unsigned const value) {
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
                return just (16 * value + digit);
            }

            // consume hex state [static]
            // ~~~~~~~~~~~~~~~~~
            template <typename Callbacks>
            auto string_matcher<Callbacks>::consume_hex_state (unsigned const hex,
                                                               enum state const state,
                                                               char32_t const code_point)
                -> maybe<std::tuple<unsigned, enum state>> {

                    return hex_value (code_point, hex) >>=
                           [state] (unsigned value) {
                               PSTORE_ASSERT (value <= std::numeric_limits<std::uint16_t>::max ());
                               auto next_state = state;
                               switch (state) {
                               case hex1_state: next_state = hex2_state; break;
                               case hex2_state: next_state = hex3_state; break;
                               case hex3_state: next_state = hex4_state; break;
                               case hex4_state: next_state = normal_char_state; break;

                               case start_state:
                               case normal_char_state:
                               case escape_state:
                               case done_state:
                                   PSTORE_ASSERT (false);
                                   return nothing<std::tuple<unsigned, enum state>> ();
                               }

                               return just (std::make_tuple (value, next_state));
                           };
                }

            // consume escape state [static]
            // ~~~~~~~~~~~~~~~~~~~~
            template <typename Callbacks>
            auto string_matcher<Callbacks>::consume_escape_state (char32_t code_point,
                                                                  appender & app)
                -> std::tuple<state, error_code> {

                auto decode = [] (char32_t cp) {
                    state next_state = normal_char_state;
                    switch (cp) {
                    case '"': cp = '"'; break;
                    case '\\': cp = '\\'; break;
                    case '/': cp = '/'; break;
                    case 'b': cp = '\b'; break;
                    case 'f': cp = '\f'; break;
                    case 'n': cp = '\n'; break;
                    case 'r': cp = '\r'; break;
                    case 't': cp = '\t'; break;
                    case 'u': next_state = hex1_state; break;
                    default: return nothing<std::tuple<char32_t, state>> ();
                    }
                    return just (std::make_tuple (cp, next_state));
                };

                auto append = [&app] (std::tuple<char32_t, state> const & s) {
                    char32_t const cp = std::get<0> (s);
                    state const next_state = std::get<1> (s);
                    PSTORE_ASSERT (next_state == normal_char_state || next_state == hex1_state);
                    if (next_state == normal_char_state) {
                        if (!app.append32 (cp)) {
                            return nothing<state> ();
                        }
                    }
                    return just (next_state);
                };

                maybe<state> const x = decode (code_point) >>= append;
                return x ? std::make_tuple (*x, error_code::none)
                         : std::make_tuple (normal_char_state, error_code::invalid_escape_char);
            }

            // consume
            // ~~~~~~~
            template <typename Callbacks>
            std::pair<typename matcher<Callbacks>::pointer, bool>
            string_matcher<Callbacks>::consume (parser<Callbacks> & parser, maybe<char> ch) {
                if (!ch) {
                    this->set_error (parser, error_code::expected_close_quote);
                    return {nullptr, true};
                }

                if (maybe<char32_t> const code_point =
                        decoder_.get (static_cast<std::uint8_t> (*ch))) {
                    switch (this->get_state ()) {
                    // Matches the opening quote.
                    case start_state:
                        if (*code_point == '"') {
                            PSTORE_ASSERT (!app_.has_high_surrogate ());
                            this->set_state (normal_char_state);
                        } else {
                            this->set_error (parser, error_code::expected_token);
                        }
                        break;
                    case normal_char_state: {
                        auto const normal_resl =
                            string_matcher::consume_normal_state (parser, *code_point, app_);
                        this->set_state (std::get<0> (normal_resl));
                        this->set_error (parser, std::get<std::error_code> (normal_resl));
                    } break;

                    case escape_state: {
                        auto const escape_resl =
                            string_matcher::consume_escape_state (*code_point, app_);
                        this->set_state (std::get<0> (escape_resl));
                        this->set_error (parser, std::get<1> (escape_resl));
                    } break;

                    case hex1_state: hex_ = 0; PSTORE_FALLTHROUGH;
                    case hex2_state:
                    case hex3_state:
                    case hex4_state: {
                        maybe<std::tuple<unsigned, state>> const hex_resl =
                            string_matcher::consume_hex_state (
                                hex_, static_cast<state> (this->get_state ()), *code_point);
                        if (!hex_resl) {
                            this->set_error (parser, error_code::invalid_hex_char);
                            break;
                        }
                        hex_ = std::get<0> (*hex_resl);
                        state const next_state = std::get<1> (*hex_resl);
                        this->set_state (next_state);
                        // We're done with the hex characters and are switching back to the "normal"
                        // state. The means that we can add the accumulated code-point (in hex_) to
                        // the string.
                        if (next_state == normal_char_state) {
                            if (!app_.append16 (static_cast<char16_t> (hex_))) {
                                this->set_error (parser, error_code::bad_unicode_code_point);
                            }
                        }
                    } break;

                    case done_state: PSTORE_ASSERT (false); break;
                    }
                }
                return {nullptr, true};
            }

            //*                          *
            //*  __ _ _ _ _ _ __ _ _  _  *
            //* / _` | '_| '_/ _` | || | *
            //* \__,_|_| |_| \__,_|\_, | *
            //*                    |__/  *
            //-MARK:array
            template <typename Callbacks>
            class array_matcher final : public matcher<Callbacks> {
            public:
                array_matcher () noexcept
                        : matcher<Callbacks> (start_state) {}

                std::pair<typename matcher<Callbacks>::pointer, bool>
                consume (parser<Callbacks> & parser, maybe<char> ch) override;

            private:
                enum state {
                    done_state = matcher<Callbacks>::done,
                    start_state,
                    first_object_state,
                    object_state,
                    comma_state,
                };

                void end_array (parser<Callbacks> & parser);
            };

            // consume
            // ~~~~~~~
            template <typename Callbacks>
            std::pair<typename matcher<Callbacks>::pointer, bool>
            array_matcher<Callbacks>::consume (parser<Callbacks> & parser, maybe<char> ch) {
                if (!ch) {
                    this->set_error (parser, error_code::expected_array_member);
                    return {nullptr, true};
                }
                char const c = *ch;
                switch (this->get_state ()) {
                case start_state:
                    PSTORE_ASSERT (c == '[');
                    if (this->set_error (parser, parser.callbacks ().begin_array ())) {
                        break;
                    }
                    this->set_state (first_object_state);
                    // Match this character and consume whitespace before the object (or close
                    // bracket).
                    return {this->make_whitespace_matcher (parser), true};

                case first_object_state:
                    if (c == ']') {
                        this->end_array (parser);
                        break;
                    }
                    PSTORE_FALLTHROUGH;
                case object_state:
                    this->set_state (comma_state);
                    return {this->make_root_matcher (parser), false};
                    break;
                case comma_state:
                    if (isspace (c)) {
                        // just consume whitespace before a comma.
                        return {this->make_whitespace_matcher (parser), false};
                    }
                    switch (c) {
                    case ',':
                        this->set_state (
                            (parser.extension_enabled (extensions::array_trailing_comma))
                                ? first_object_state
                                : object_state);
                        return {this->make_whitespace_matcher (parser), true};
                    case ']': this->end_array (parser); break;
                    default: this->set_error (parser, error_code::expected_array_member); break;
                    }
                    break;
                case done_state: PSTORE_ASSERT (false); break;
                }
                return {nullptr, true};
            }

            // end array
            // ~~~~~~~~~
            template <typename Callbacks>
            void array_matcher<Callbacks>::end_array (parser<Callbacks> & parser) {
                this->set_error (parser, parser.callbacks ().end_array ());
                this->set_state (done_state);
            }

            //*      _     _        _    *
            //*  ___| |__ (_)___ __| |_  *
            //* / _ \ '_ \| / -_) _|  _| *
            //* \___/_.__// \___\__|\__| *
            //*         |__/             *
            //-MARK:object
            template <typename Callbacks>
            class object_matcher final : public matcher<Callbacks> {
            public:
                object_matcher () noexcept
                        : matcher<Callbacks> (start_state) {}

                std::pair<typename matcher<Callbacks>::pointer, bool>
                consume (parser<Callbacks> & parser, maybe<char> ch) override;

            private:
                enum state {
                    done_state = matcher<Callbacks>::done,
                    start_state,
                    first_key_state,
                    key_state,
                    colon_state,
                    value_state,
                    comma_state,
                };

                void end_object (parser<Callbacks> & parser);
            };

            // consume
            // ~~~~~~~
            template <typename Callbacks>
            std::pair<typename matcher<Callbacks>::pointer, bool>
            object_matcher<Callbacks>::consume (parser<Callbacks> & parser, maybe<char> ch) {
                if (this->get_state () == done_state) {
                    PSTORE_ASSERT (parser.last_error ());
                    return {nullptr, true};
                }
                if (!ch) {
                    this->set_error (parser, error_code::expected_object_member);
                    return {nullptr, true};
                }
                char const c = *ch;
                switch (this->get_state ()) {
                case start_state:
                    PSTORE_ASSERT (c == '{');
                    this->set_state (first_key_state);
                    if (this->set_error (parser, parser.callbacks ().begin_object ())) {
                        break;
                    }
                    return {this->make_whitespace_matcher (parser), true};
                case first_key_state:
                    // We allow either a closing brace (to end the object) or a property name.
                    if (c == '}') {
                        this->end_object (parser);
                        break;
                    }
                    PSTORE_FALLTHROUGH;
                case key_state:
                    // Match a property name then expect a colon.
                    this->set_state (colon_state);
                    return {this->make_root_matcher (parser, true /*object key?*/), false};
                case colon_state:
                    if (isspace (c)) {
                        // just consume whitespace before the colon.
                        return {this->make_whitespace_matcher (parser), false};
                    }
                    if (c == ':') {
                        this->set_state (value_state);
                    } else {
                        this->set_error (parser, error_code::expected_colon);
                    }
                    break;
                case value_state:
                    this->set_state (comma_state);
                    return {this->make_root_matcher (parser), false};
                case comma_state:
                    if (isspace (c)) {
                        // just consume whitespace before the comma.
                        return {this->make_whitespace_matcher (parser), false};
                    }
                    if (c == ',') {
                        // Strictly conforming JSON requires a property name following a comma but
                        // we have an extension to allow an trailing comma which may be followed by
                        // the object's closing brace.
                        this->set_state (
                            (parser.extension_enabled (extensions::object_trailing_comma))
                                ? first_key_state
                                : key_state);
                        // Consume the comma and any whitespace before the close brace or property
                        // name.
                        return {this->make_whitespace_matcher (parser), true};
                    }
                    if (c == '}') {
                        this->end_object (parser);
                    } else {
                        this->set_error (parser, error_code::expected_object_member);
                    }
                    break;
                case done_state: PSTORE_ASSERT (false); break;
                }
                // No change of matcher. Consume the input character.
                return {nullptr, true};
            }

            // end object
            // ~~~~~~~~~~~
            template <typename Callbacks>
            void object_matcher<Callbacks>::end_object (parser<Callbacks> & parser) {
                this->set_error (parser, parser.callbacks ().end_object ());
                this->set_state (done_state);
            }

            //*             *
            //* __ __ _____ *
            //* \ V  V (_-< *
            //*  \_/\_//__/ *
            //*             *
            /// This matcher consumes whitespace and updates the row number in response to the
            /// various combinations of CR and LF. Supports #, //, and /* style comments as an
            /// extension.
            //-MARK:whitespace
            template <typename Callbacks>
            class whitespace_matcher final : public matcher<Callbacks> {
            public:
                whitespace_matcher () noexcept
                        : matcher<Callbacks> (body_state) {}

                std::pair<typename matcher<Callbacks>::pointer, bool>
                consume (parser<Callbacks> & parser, maybe<char> ch) override;

            private:
                enum state {
                    done_state = matcher<Callbacks>::done,
                    /// Normal whitespace scanning. The "body" is the whitespace being consumed.
                    body_state,
                    /// Handles the LF part of a Windows-style CR/LF pair.
                    crlf_state,
                    /// Consumes the contents of a single-line comment.
                    single_line_comment_state,
                    comment_start_state,
                    /// Consumes the contents of a multi-line comment.
                    multi_line_comment_body_state,
                    /// Entered when checking for the second character of the '*/' pair.
                    multi_line_comment_ending_state,
                    /// Handles the LF part of a Windows-style CR/LF pair inside a multi-line
                    /// comment.
                    multi_line_comment_crlf_state,
                };

                std::pair<typename matcher<Callbacks>::pointer, bool>
                consume_body (parser<Callbacks> & parser, char c);

                std::pair<typename matcher<Callbacks>::pointer, bool>
                consume_comment_start (parser<Callbacks> & parser, char c);

                std::pair<typename matcher<Callbacks>::pointer, bool>
                multi_line_comment_body (parser<Callbacks> & parser, char c);

                void cr (parser<Callbacks> & parser, state next) {
                    PSTORE_ASSERT (this->get_state () == multi_line_comment_body_state ||
                                   this->get_state () == body_state);
                    parser.advance_row ();
                    this->set_state (next);
                }
                void lf (parser<Callbacks> & parser) { parser.advance_row (); }

                /// Processes the second character of a Windows-style CR/LF pair. Returns true if
                /// the character shoud be treated as whitespace.
                bool crlf (parser<Callbacks> & parser, char c) {
                    if (c != details::char_set::lf) {
                        return false;
                    }
                    parser.reset_column ();
                    return true;
                }
            };

            // consume
            // ~~~~~~~
            template <typename Callbacks>
            std::pair<typename matcher<Callbacks>::pointer, bool>
            whitespace_matcher<Callbacks>::consume (parser<Callbacks> & parser, maybe<char> ch) {
                if (!ch) {
                    this->set_state (done_state);
                } else {
                    char const c = *ch;
                    switch (this->get_state ()) {
                    // Handles the LF part of a Windows-style CR/LF pair.
                    case crlf_state:
                        this->set_state (body_state);
                        if (crlf (parser, c)) {
                            break;
                        }
                        PSTORE_FALLTHROUGH;
                    case body_state: return this->consume_body (parser, c);
                    case comment_start_state: return this->consume_comment_start (parser, c);

                    case multi_line_comment_ending_state:
                        PSTORE_ASSERT (parser.extension_enabled (extensions::multi_line_comments));
                        this->set_state (c == details::char_set::slash
                                             ? body_state
                                             : multi_line_comment_body_state);
                        break;

                    case multi_line_comment_crlf_state:
                        this->set_state (multi_line_comment_body_state);
                        if (crlf (parser, c)) {
                            break;
                        }
                        PSTORE_FALLTHROUGH;
                    case multi_line_comment_body_state:
                        return this->multi_line_comment_body (parser, c);
                    case single_line_comment_state:
                        PSTORE_ASSERT (
                            parser.extension_enabled (extensions::bash_comments) ||
                            parser.extension_enabled (extensions::single_line_comments) ||
                            parser.extension_enabled (extensions::multi_line_comments));
                        if (c == details::char_set::cr || c == details::char_set::lf) {
                            // This character marks a bash/single-line comment end. Go back to
                            // normal whitespace handling. Retry with the same character.
                            this->set_state (body_state);
                            return {nullptr, false};
                        }
                        // Just consume the character.
                        break;

                    case done_state: PSTORE_ASSERT (false); break;
                    }
                }
                return {nullptr, true};
            }

            // consume body
            // ~~~~~~~~~~~~
            template <typename Callbacks>
            std::pair<typename matcher<Callbacks>::pointer, bool>
            whitespace_matcher<Callbacks>::consume_body (parser<Callbacks> & parser, char c) {
                auto const stop_retry = [this] () {
                    // Stop, pop this matcher, and retry with the same character.
                    this->set_state (done_state);
                    return std::pair<typename matcher<Callbacks>::pointer, bool>{nullptr, false};
                };

                using details::char_set;
                switch (c) {
                case char_set::space: break; // Just consume.
                case char_set::tab:
                    // TODO: tab expansion.
                    break;
                case char_set::cr: this->cr (parser, crlf_state); break;
                case char_set::lf: this->lf (parser); break;
                case char_set::hash:
                    if (!parser.extension_enabled (extensions::bash_comments)) {
                        return stop_retry ();
                    }
                    this->set_state (single_line_comment_state);
                    break;
                case char_set::slash:
                    if (!parser.extension_enabled (extensions::single_line_comments) &&
                        !parser.extension_enabled (extensions::multi_line_comments)) {
                        return stop_retry ();
                    }
                    this->set_state (comment_start_state);
                    break;
                default: return stop_retry ();
                }
                return {nullptr, true}; // Consume this character.
            }

            // consume comment start
            // ~~~~~~~~~~~~~~~~~~~~~
            /// We've already seen an initial slash ('/') which could mean one of three
            /// things:
            ///   - the start of a single-line // comment
            ///   - the start of a multi-line /* */ comment
            ///   - just a random / character.
            /// This function handles the character after that initial slash to determine which of
            /// the three it is.
            template <typename Callbacks>
            std::pair<typename matcher<Callbacks>::pointer, bool>
            whitespace_matcher<Callbacks>::consume_comment_start (parser<Callbacks> & parser,
                                                                  char c) {
                using details::char_set;
                if (c == char_set::slash &&
                    parser.extension_enabled (extensions::single_line_comments)) {
                    this->set_state (single_line_comment_state);
                } else if (c == char_set::star &&
                           parser.extension_enabled (extensions::multi_line_comments)) {
                    this->set_state (multi_line_comment_body_state);
                } else {
                    this->set_error (parser, error_code::expected_token);
                }
                return {nullptr, true}; // Consume this character.
            }

            // multi line comment body
            // ~~~~~~~~~~~~~~~~~~~~~~~
            /// Similar to consume_body() except that the commented characters are consumed as well
            /// as whitespace. We're looking to see a star ('*') character which may indicate the
            /// end of the multi-line comment.
            template <typename Callbacks>
            std::pair<typename matcher<Callbacks>::pointer, bool>
            whitespace_matcher<Callbacks>::multi_line_comment_body (parser<Callbacks> & parser,
                                                                    char c) {
                using details::char_set;
                PSTORE_ASSERT (parser.extension_enabled (extensions::multi_line_comments));
                PSTORE_ASSERT (this->get_state () == multi_line_comment_body_state);
                switch (c) {
                case char_set::star:
                    // This could be a standalone star character or be followed by a slash
                    // to end the multi-line comment.
                    this->set_state (multi_line_comment_ending_state);
                    break;
                case char_set::cr: this->cr (parser, multi_line_comment_crlf_state); break;
                case char_set::lf: this->lf (parser); break;
                case char_set::tab: break; // TODO: tab expansion.
                default: break;            // Just consume.
                }
                return {nullptr, true}; // Consume this character.
            }

            //*           __  *
            //*  ___ ___ / _| *
            //* / -_) _ \  _| *
            //* \___\___/_|   *
            //*               *
            //-MARK:eof
            template <typename Callbacks>
            class eof_matcher final : public matcher<Callbacks> {
            public:
                eof_matcher () noexcept
                        : matcher<Callbacks> (start_state) {}

                std::pair<typename matcher<Callbacks>::pointer, bool>
                consume (parser<Callbacks> & parser, maybe<char> ch) override;

            private:
                enum state {
                    done_state = matcher<Callbacks>::done,
                    start_state,
                };
            };

            // consume
            // ~~~~~~~
            template <typename Callbacks>
            std::pair<typename matcher<Callbacks>::pointer, bool>
            eof_matcher<Callbacks>::consume (parser<Callbacks> & parser, maybe<char> const ch) {
                if (ch) {
                    this->set_error (parser, error_code::unexpected_extra_input);
                } else {
                    this->set_state (done_state);
                }
                return {nullptr, true};
            }

            //*               _                _      _             *
            //*  _ _ ___  ___| |_   _ __  __ _| |_ __| |_  ___ _ _  *
            //* | '_/ _ \/ _ \  _| | '  \/ _` |  _/ _| ' \/ -_) '_| *
            //* |_| \___/\___/\__| |_|_|_\__,_|\__\__|_||_\___|_|   *
            //*                                                     *
            //-MARK:root
            template <typename Callbacks>
            class root_matcher final : public matcher<Callbacks> {
            public:
                explicit constexpr root_matcher (bool const object_key = false) noexcept
                        : matcher<Callbacks> (start_state)
                        , object_key_{object_key} {}

                std::pair<typename matcher<Callbacks>::pointer, bool>
                consume (parser<Callbacks> & parser, maybe<char> ch) override;

            private:
                enum state {
                    done_state = matcher<Callbacks>::done,
                    start_state,
                    new_token_state,
                };
                bool const object_key_;
            };

            // consume
            // ~~~~~~~
            template <typename Callbacks>
            std::pair<typename matcher<Callbacks>::pointer, bool>
            root_matcher<Callbacks>::consume (parser<Callbacks> & parser, maybe<char> ch) {
                if (!ch) {
                    this->set_error (parser, error_code::expected_token);
                    return {nullptr, true};
                }

                switch (this->get_state ()) {
                case start_state:
                    this->set_state (new_token_state);
                    return {this->make_whitespace_matcher (parser), false};

                case new_token_state: {
                    if (object_key_ && *ch != '"') {
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
                        return {this->template make_terminal_matcher<number_matcher<Callbacks>> (
                                    parser),
                                false};
                    case '"':
                        return {this->template make_terminal_matcher<string_matcher<Callbacks>> (
                                    parser, &parser.string_, object_key_),
                                false};
                    case 't':
                        return {
                            this->template make_terminal_matcher<true_token_matcher<Callbacks>> (
                                parser, "true"),
                            false};
                    case 'f':
                        return {
                            this->template make_terminal_matcher<false_token_matcher<Callbacks>> (
                                parser, "false"),
                            false};
                    case 'n':
                        return {
                            this->template make_terminal_matcher<null_token_matcher<Callbacks>> (
                                parser, "null"),
                            false};
                    case '[':
                        return {
                            typename matcher<Callbacks>::pointer (new array_matcher<Callbacks> ()),
                            false};
                    case '{':
                        return {
                            typename matcher<Callbacks>::pointer (new object_matcher<Callbacks> ()),
                            false};
                    default:
                        this->set_error (parser, error_code::expected_token);
                        return {nullptr, true};
                    }
                } break;
                case done_state: PSTORE_ASSERT (false); break;
                }
                PSTORE_ASSERT (false); // unreachable.
                return {nullptr, true};
            }


            //*     _           _     _                 _                          *
            //*  __(_)_ _  __ _| |___| |_ ___ _ _    __| |_ ___ _ _ __ _ __ _ ___  *
            //* (_-< | ' \/ _` | / -_)  _/ _ \ ' \  (_-<  _/ _ \ '_/ _` / _` / -_) *
            //* /__/_|_||_\__, |_\___|\__\___/_||_| /__/\__\___/_| \__,_\__, \___| *
            //*           |___/                                         |___/      *
            //-MARK:singleton storage
            template <typename Callbacks>
            struct singleton_storage {
                template <typename T>
                struct storage {
                    using type = typename std::aligned_storage<sizeof (T), alignof (T)>::type;
                };

                typename storage<eof_matcher<Callbacks>>::type eof;
                typename storage<whitespace_matcher<Callbacks>>::type trailing_ws;
                typename storage<root_matcher<Callbacks>>::type root;

                using matcher_characteristics =
                    characteristics<number_matcher<Callbacks>, string_matcher<Callbacks>,
                                    true_token_matcher<Callbacks>, false_token_matcher<Callbacks>,
                                    null_token_matcher<Callbacks>, whitespace_matcher<Callbacks>>;

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
                static void get () {}
            };

        } // end namespace details


        // (ctor)
        // ~~~~~~
        template <typename Callbacks>
        parser<Callbacks>::parser (Callbacks callbacks, extensions const extensions)
                : callbacks_ (std::move (callbacks))
                , extensions_{extensions} {

            using mpointer = typename matcher::pointer;
            using deleter = typename mpointer::deleter_type;
            // The EOF matcher is placed at the bottom of the stack to ensure that the input JSON
            // ends after a single top-level object.
            stack_.push (mpointer (new (&singletons_->eof) details::eof_matcher<Callbacks>{},
                                   deleter{false}));
            // We permit whitespace after the top-level object.
            stack_.push (mpointer (new (&singletons_->trailing_ws)
                                       details::whitespace_matcher<Callbacks>{},
                                   deleter{false}));
            stack_.push (this->make_root_matcher ());
        }

        // make root matcher
        // ~~~~~~~~~~~~~~~~~
        template <typename Callbacks>
        auto parser<Callbacks>::make_root_matcher (bool object_key) -> pointer {
            using root_matcher = details::root_matcher<Callbacks>;
            return pointer (new (&singletons_->root) root_matcher (object_key),
                            typename pointer::deleter_type{false});
        }

        // make whitespace matcher
        // ~~~~~~~~~~~~~~~~~~~~~~~
        template <typename Callbacks>
        auto parser<Callbacks>::make_whitespace_matcher () -> pointer {
            using whitespace_matcher = details::whitespace_matcher<Callbacks>;
            return this->make_terminal_matcher<whitespace_matcher> ();
        }

        // get terminal storage
        // ~~~~~~~~~~~~~~~~~~~~
        template <typename Callbacks>
        void const * parser<Callbacks>::get_terminal_storage () const noexcept {
            return &singletons_->terminal;
        }

        // make terminal matcher
        // ~~~~~~~~~~~~~~~~~~~~~
        template <typename Callbacks>
        template <typename Matcher, typename... Args>
        auto parser<Callbacks>::make_terminal_matcher (Args &&... args) -> pointer {
            static_assert (sizeof (Matcher) <= sizeof (decltype (singletons_->terminal)),
                           "terminal storage is not large enough for Matcher type");
            static_assert (alignof (Matcher) <= alignof (decltype (singletons_->terminal)),
                           "terminal storage is not sufficiently aligned for Matcher type");

            return pointer (new (&singletons_->terminal) Matcher (std::forward<Args> (args)...),
                            typename pointer::deleter_type{false});
        }

        // input
        // ~~~~~
        template <typename Callbacks>
        template <typename SpanType>
        auto parser<Callbacks>::input (SpanType const & span) -> parser & {
            static_assert (
                std::is_same<typename std::remove_cv<typename SpanType::element_type>::type,
                             char>::value,
                "span element type must be char");
            return this->input (std::begin (span), std::end (span));
        }

        template <typename Callbacks>
        template <typename InputIterator>
        auto parser<Callbacks>::input (InputIterator first, InputIterator last) -> parser & {
            static_assert (
                std::is_same<typename std::remove_cv<
                                 typename std::iterator_traits<InputIterator>::value_type>::type,
                             char>::value,
                "iterator value_type must be char");
            if (error_) {
                return *this;
            }
            while (first != last) {
                PSTORE_ASSERT (!stack_.empty ());
                auto & handler = stack_.top ();
                auto res = handler->consume (*this, just (*first));
                if (handler->is_done ()) {
                    if (error_) {
                        break;
                    }
                    stack_.pop (); // release the topmost matcher object.
                }

                if (res.first != nullptr) {
                    if (stack_.size () > max_stack_depth_) {
                        // We've already hit the maximum allowed parse stack depth. Reject this new
                        // matcher.
                        PSTORE_ASSERT (!error_);
                        error_ = make_error_code (error_code::nesting_too_deep);
                        break;
                    }

                    stack_.push (std::move (res.first));
                }
                // If we're matching this character, advance the column number and increment the
                // iterator.
                if (res.second) {
                    // Increment the column number if this is _not_ a UTF-8 continuation character.
                    if (utf::is_utf_char_start (*first)) {
                        this->advance_column ();
                    }
                    ++first;
                }
            }
            return *this;
        }

        // eof
        // ~~~
        template <typename Callbacks>
        auto parser<Callbacks>::eof () -> result_type {
            while (!stack_.empty () && !has_error ()) {
                auto & handler = stack_.top ();
                auto res = handler->consume (*this, nothing<char> ());
                PSTORE_ASSERT (handler->is_done ());
                PSTORE_ASSERT (res.second);
                stack_.pop (); // release the topmost matcher object.
            }
            return has_error () ? details::default_return<result_type>::get ()
                                : this->callbacks ().result ();
        }

    } // namespace json
} // namespace pstore

#endif // PSTORE_JSON_JSON_HPP
