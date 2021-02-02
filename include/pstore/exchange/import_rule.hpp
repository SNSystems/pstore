//*  _                            _                _       *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __ _   _| | ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '__| | | | |/ _ \ *
//* | | | | | | | |_) | (_) | |  | |_  | |  | |_| | |  __/ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_|   \__,_|_|\___| *
//*             |_|                                        *
//===- include/pstore/exchange/import_rule.hpp ----------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file import_rule.hpp
/// \brief Declares the "rule" class which models a production in the import grammar and its
/// interface with the JSON parser.

#ifndef PSTORE_EXCHANGE_IMPORT_RULE_HPP
#define PSTORE_EXCHANGE_IMPORT_RULE_HPP

#include <sstream>

#include "pstore/exchange/import_context.hpp"
#include "pstore/os/logging.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            template <typename T>
            using not_null = gsl::not_null<T>;

            //*           _      *
            //*  _ _ _  _| |___  *
            //* | '_| || | / -_) *
            //* |_|  \_,_|_\___| *
            //*                  *
            //-MARK: rule
            /// This class models a production rule in the import grammar. Subclasses are used to
            /// specialize for different classes of input (such as compilations, fragments, and so
            /// on).
            class rule {
            public:
                explicit rule (not_null<context *> const context) noexcept
                        : context_{context} {}
                rule (rule const &) = delete;
                rule (rule &&) noexcept = delete;

                virtual ~rule ();

                rule & operator= (rule const &) = delete;
                rule & operator= (rule &&) noexcept = delete;

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
                /// arguments are forwarded to the T constructor in addition to the parse stack
                /// itself.
                ///
                /// \tparam T  The type of the new rule to instantiate.
                /// \tparam Args The types of the arguments for the constructor of type T.
                /// \param args The parameters to be passed to the contructor of the new instance of
                /// type T.
                template <typename T, typename... Args>
                std::error_code push (Args... args) {
                    context_->stack.push (std::make_unique<T> (context_, args...));
                    log_top (true);
                    return {};
                }

            protected:
                /// Removes the top-most element from the parse-stack and replaces it with a newly
                /// instantiate object.
                ///
                /// \tparam T  The type of the new rule to instantiate.
                /// \tparam Args The types of the arguments for the constructor of type T.
                /// \param args The parameters to be passed to the contructor of the new instance of
                /// type T.
                template <typename T, typename... Args>
                std::error_code replace_top (Args... args) {
                    auto p = std::make_unique<T> (context_, args...);
                    log_top (false);
                    // Remember the context pointer before we destroy 'this'.
                    auto * const context = this->get_context ();
                    context->stack.pop (); // Destroys this object.
                    context->stack.push (std::move (p));
                    log_top (true);
                    return {};
                }

                /// Removes the top-most element from the parse-stack and returns "no-error".
                /// This member function is usually called to signal the end of the current grammar
                /// rule.
                std::error_code pop () {
                    log_top (false);
                    context_->stack.pop ();
                    return {};
                }

                context * get_context () noexcept { return context_; }

            private:
                void log_top (bool const is_push) const {
                    if (logging_enabled ()) {
                        log_top_impl (is_push);
                    }
                }

                void log_top_impl (bool is_push) const;

                not_null<context *> const context_;
            };


            //-MARK: callbacks
            /// Implements the callback interface required by the JSON parser. Each member function
            /// forwards to the top-most element on the parse-stack (an instance of a subclass of
            /// pstore::exchange::import::rule).
            class callbacks {
            public:
                using result_type = void;
                result_type result () {}

                template <typename Rule, typename... Args>
                static callbacks make (gsl::not_null<database *> const db, Args... args) {
                    auto ctxt = std::make_shared<context> (db);
                    return {ctxt, std::make_unique<Rule> (ctxt.get (), args...)};
                }

                std::shared_ptr<context> & get_context () { return context_; }

                std::error_code int64_value (std::int64_t const v) {
                    return top ()->int64_value (v);
                }
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
                callbacks (std::shared_ptr<context> const & ctxt, std::unique_ptr<rule> && root)
                        : context_{ctxt} {
                    context_->stack.push (std::move (root));
                }

                std::unique_ptr<rule> & top () {
                    PSTORE_ASSERT (!context_->stack.empty ());
                    return context_->stack.top ();
                }
                std::shared_ptr<context> context_;
            };

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_RULE_HPP
