//*  _                            _                _       *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __ _   _| | ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '__| | | | |/ _ \ *
//* | | | | | | | |_) | (_) | |  | |_  | |  | |_| | |  __/ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_|   \__,_|_|\___| *
//*             |_|                                        *
//===- include/pstore/exchange/import_rule.hpp ----------------------------===//
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
#ifndef PSTORE_EXCHANGE_IMPORT_RULE_HPP
#define PSTORE_EXCHANGE_IMPORT_RULE_HPP

#include <sstream>
#include <stack>
#include <system_error>

#include "pstore/os/logging.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace exchange {

        template <typename T>
        using not_null = gsl::not_null<T>;

        //*  _                     _              _      *
        //* (_)_ __  _ __  ___ _ _| |_   _ _ _  _| |___  *
        //* | | '  \| '_ \/ _ \ '_|  _| | '_| || | / -_) *
        //* |_|_|_|_| .__/\___/_|  \__| |_|  \_,_|_\___| *
        //*         |_|                                  *
        //-MARK: import rule
        class import_rule {
        public:
            using parse_stack = std::stack<std::unique_ptr<import_rule>>;
            using parse_stack_pointer = not_null<parse_stack *>;

            explicit import_rule (parse_stack_pointer const stack) noexcept
                    : stack_{stack} {}
            import_rule (import_rule const &) = delete;
            import_rule (import_rule &&) noexcept = delete;

            virtual ~import_rule ();

            import_rule & operator= (import_rule const &) = delete;
            import_rule & operator= (import_rule &&) noexcept = delete;

            virtual gsl::czstring name () const noexcept = 0;

            virtual std::error_code int64_value (std::int64_t v);
            virtual std::error_code uint64_value (std::uint64_t v);
            virtual std::error_code double_value (double v);
            virtual std::error_code string_value (std::string const & v);
            virtual std::error_code boolean_value (bool v);
            virtual std::error_code null_value ();
            virtual std::error_code begin_array ();
            virtual std::error_code end_array ();
            virtual std::error_code begin_object ();
            virtual std::error_code key (std::string const & k);
            virtual std::error_code end_object ();

            /// Creates an instance of type T and pushes it onto the parse stack. The provided
            /// arguments are forwarded to the T constructor in addition to the parse stack itself.
            template <typename T, typename... Args>
            std::error_code push (Args... args) {
                stack_->push (std::make_unique<T> (stack_, args...));
                log_top (true);
                return {};
            }

        protected:
            template <typename T, typename... Args>
            std::error_code replace_top (Args... args) {
                auto p = std::make_unique<T> (stack_, args...);
                auto const stack = stack_;
                log_top (false);
                stack->pop (); // Destroys this object.

                stack->push (std::move (p));
                log_top (true);
                return {};
            }

            std::error_code pop () {
                log_top (false);
                stack_->pop ();
                return {};
            }

        private:
            static constexpr auto trace_indent = std::size_t{2};
            static std::string indent (parse_stack const & stack) {
                return std::string (stack.size () * trace_indent, ' ');
            }

            inline void log_top (bool is_push) const {
                if (logging::enabled ()) {
                    log_top_impl (is_push);
                }
            }

            void log_top_impl (bool is_push) const;

            parse_stack_pointer const stack_;
        };


        //-MARK: callbacks
        class callbacks {
        public:
            using result_type = void;
            result_type result () {}

            template <typename Rule, typename... Args>
            static callbacks make (Args... args) {
                auto stack = std::make_shared<import_rule::parse_stack> ();
                return {stack, std::make_unique<Rule> (stack.get (), args...)};
            }

            std::error_code int64_value (std::int64_t const v) { return top ()->int64_value (v); }
            std::error_code uint64_value (std::uint64_t const v) {
                return top ()->uint64_value (v);
            }
            std::error_code double_value (double const v) { return top ()->double_value (v); }
            std::error_code string_value (std::string const & v) {
                return top ()->string_value (v);
            }
            std::error_code boolean_value (bool const v) { return top ()->boolean_value (v); }
            std::error_code null_value () { return top ()->null_value (); }
            std::error_code begin_array () { return top ()->begin_array (); }
            std::error_code end_array () { return top ()->end_array (); }
            std::error_code begin_object () { return top ()->begin_object (); }
            std::error_code key (std::string const & k) { return top ()->key (k); }
            std::error_code end_object () { return top ()->end_object (); }

        private:
            callbacks (std::shared_ptr<import_rule::parse_stack> const & stack,
                       std::unique_ptr<import_rule> && root)
                    : stack_{stack} {
                stack_->push (std::move (root));
            }

            std::unique_ptr<import_rule> & top () {
                assert (!stack_->empty ());
                return stack_->top ();
            }
            std::shared_ptr<import_rule::parse_stack> stack_;
        };

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_RULE_HPP
