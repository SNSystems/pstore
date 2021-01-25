//*                                                _   _ _             *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | | (_)_ __   ___  *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | | | | '_ \ / _ \ *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | | | | | | |  __/ *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| |_|_|_| |_|\___| *
//*                                                                    *
//===- include/pstore/command_line/command_line.hpp -----------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_COMMAND_LINE_HPP
#define PSTORE_COMMAND_LINE_COMMAND_LINE_HPP

#include "pstore/command_line/help.hpp"
#include "pstore/command_line/modifiers.hpp"
#include "pstore/command_line/tchar.hpp"
#include "pstore/os/path.hpp"

namespace pstore {
    namespace command_line {

        namespace details {

            maybe<option *> lookup_nearest_option (std::string const & arg,
                                                   option::options_container const & all_options);

            bool starts_with (std::string const & s, gsl::czstring prefix);
            maybe<option *> find_handler (std::string const & name);

            // check_for_missing
            // ~~~~~~~~~~~~~~~~~
            /// Makes sure that all of the required args have been specified.
            template <typename ErrorStream>
            bool check_for_missing (std::string const & program_name, ErrorStream & errs) {
                using str = stream_trait<typename ErrorStream::char_type>;
                using pstore::command_line::num_occurrences_flag;
                using pstore::command_line::option;

                bool ok = true;
                auto positional_missing = 0U;

                for (option const * const opt : option::all ()) {
                    switch (opt->get_num_occurrences_flag ()) {
                    case num_occurrences_flag::required:
                    case num_occurrences_flag::one_or_more:
                        if (opt->get_num_occurrences () == 0U) {
                            if (opt->is_positional ()) {
                                ++positional_missing;
                            } else {
                                errs << str::out_string (program_name)
                                     << str::out_text (": option '")
                                     << str::out_string (opt->name ())
                                     << str::out_text ("' must be specified at least once\n");
                            }
                            ok = false;
                        }
                        break;
                    case num_occurrences_flag::optional:
                    case num_occurrences_flag::zero_or_more: break;
                    }
                }

                if (positional_missing == 1U) {
                    errs << str::out_string (program_name)
                         << str::out_text (": a positional argument was missing\n");
                } else if (positional_missing > 1U) {
                    errs << str::out_string (program_name) << positional_missing
                         << str::out_text (": positional arguments are missing\n");
                }

                return ok;
            }

            // report_unknown_option
            // ~~~~~~~~~~~~~~~~~~~~~
            template <typename ErrorStream>
            void report_unknown_option (std::string const & program_name,
                                        std::string const & arg_name, std::string const & value,
                                        ErrorStream & errs) {
                using str = stream_trait<typename ErrorStream::char_type>;
                errs << str::out_string (program_name)
                     << str::out_text (": Unknown command line argument '")
                     << str::out_string (arg_name) << str::out_text ("'\n");

                if (maybe<option *> const best_option =
                        lookup_nearest_option (arg_name, option::all ())) {
                    std::string nearest_string = (*best_option)->name ();
                    gsl::czstring const dashes = utf::length (nearest_string) < 2U ? "-" : "--";
                    if (!value.empty ()) {
                        nearest_string += '=';
                        nearest_string += value;
                    }
                    errs << str::out_text ("Did you mean '") << str::out_string (dashes)
                         << str::out_string (nearest_string) << str::out_text ("'?\n");
                }
            }

            template <typename ErrorStream>
            void report_unknown_option (std::string const & program_name,
                                        std::string const & arg_name,
                                        maybe<std::string> const & value, ErrorStream & errs) {
                report_unknown_option (program_name, arg_name, value ? *value : "", errs);
            }


            bool argument_is_positional (std::string const & arg_name);
            bool handler_takes_argument (maybe<option *> handler);
            bool handler_set_value (maybe<option *> handler, std::string const & value);

            /// Splits the name and possible argument values from an argument string.
            ///
            /// A string prefixed with a double-dash may include an optional value preceeded
            /// by an equals sign. This function splits out the leading dash or double dash and
            /// the optional value to yield the option name and value.
            ///
            /// \param arg A command-line argument string.
            /// \returns A tuple containing the argument name (shorn or leading dashes) and a
            ///          value string if one was present.
            std::tuple<std::string, maybe<std::string>> get_option_and_value (std::string arg);

            /// A simple wrapper for a bool where as soon as StickTo is assigned, subsequent
            /// assignments are ignored.
            template <bool StickTo = false>
            class sticky_bool {
            public:
                static constexpr auto stick_to = StickTo;

                explicit constexpr sticky_bool (bool const v) noexcept
                        : v_{v} {}
                sticky_bool (sticky_bool const &) noexcept = default;
                sticky_bool (sticky_bool &&) noexcept = default;
                ~sticky_bool () noexcept = default;

                sticky_bool & operator= (sticky_bool const & other) = default;
                sticky_bool & operator= (sticky_bool && other) noexcept = default;

                sticky_bool & operator= (bool const b) noexcept {
                    if (v_ != stick_to) {
                        v_ = b;
                    }
                    return *this;
                }

                constexpr bool get () const noexcept { return v_; }
                explicit constexpr operator bool () const noexcept { return get (); }

            private:
                bool v_;
            };

            template <typename ErrorStream>
            auto record_value_if_available (maybe<option *> handler,
                                            maybe<std::string> const & value,
                                            std::string const & program_name, ErrorStream & errs)
                -> std::tuple<maybe<option *>, bool> {
                using str = stream_trait<typename ErrorStream::char_type>;
                bool ok = true;
                if ((*handler)->takes_argument ()) {
                    if (value) {
                        if (!handler_set_value (handler, *value)) {
                            errs << str::out_string (program_name)
                                 << str::out_text (": Unknown value '") << str::out_string (*value)
                                 << str::out_text ("'");
                            ok = false;
                        }
                        handler.reset ();
                    } else {
                        // The option takes an argument but we haven't yet seen the value
                        // string.
                    }
                } else {
                    if (value) {
                        // We got a value but don't want one.
                        errs << str::out_string (program_name) << str::out_text (": Argument '")
                             << str::out_string ((*handler)->name ())
                             << str::out_text ("' does not take a value\n");
                        ok = false;
                    } else {
                        ok = (*handler)->add_occurrence ();
                        handler.reset ();
                    }
                }
                return std::make_tuple (std::move (handler), ok);
            }

            // process_single_dash
            // ~~~~~~~~~~~~~~~~~~~
            template <typename ErrorStream>
            std::tuple<maybe<option *>, bool> process_single_dash (std::string arg_name,
                                                                   std::string const & program_name,
                                                                   ErrorStream & errs) {
                PSTORE_ASSERT (starts_with (arg_name, "-"));
                arg_name.erase (0, 1U); // Remove the leading dash.

                auto handler = nothing<option *> ();
                sticky_bool<false> ok{true};
                while (ok && !arg_name.empty ()) {
                    char const name[2]{arg_name[0], '\0'};
                    handler = find_handler (name);
                    if (!handler || (*handler)->is_positional ()) {
                        report_unknown_option (program_name, name, nothing<std::string> (), errs);
                        ok = false;
                        break;
                    }

                    if ((*handler)->takes_argument ()) {
                        arg_name.erase (0, 1U);
                        if (arg_name.length () == 0U) {
                            // No value was supplied immediately after the argument name. It
                            // could be the next argument.
                            break;
                        }
                        ok = handler_set_value (handler, arg_name);
                        arg_name.clear ();
                    } else {
                        arg_name.erase (0, 1U);
                        ok = (*handler)->add_occurrence ();
                    }
                    handler.reset ();
                }
                return std::make_tuple (std::move (handler), ok.get ());
            }

            // parse_option_arguments
            // ~~~~~~~~~~~~~~~~~~~~~~
            template <typename InputIterator, typename ErrorStream>
            std::tuple<InputIterator, bool>
            parse_option_arguments (InputIterator first_arg, InputIterator last_arg,
                                    std::string const & program_name, ErrorStream & errs) {
                using str = stream_trait<typename ErrorStream::char_type>;
                auto value = nothing<std::string> ();
                auto handler = nothing<option *> ();
                sticky_bool<false> ok{true};

                for (; first_arg != last_arg; ++first_arg) {
                    std::string arg_name = *first_arg;
                    // Is this the argument for the preceeding switch?
                    if (handler_takes_argument (handler)) {
                        ok = handler_set_value (handler, arg_name);
                        handler.reset ();
                        continue;
                    }

                    // A double-dash argument on its own indicates that the following are
                    // positional arguments.
                    if (arg_name == "--") {
                        ++first_arg; // swallow this argument.
                        break;
                    }
                    // If this argument has no leading dash, this and the following are
                    // positional arguments.
                    if (argument_is_positional (arg_name)) {
                        break;
                    }

                    if (starts_with (arg_name, "--")) {
                        std::tie (arg_name, value) = get_option_and_value (arg_name);

                        handler = find_handler (arg_name);
                        if (!handler || (*handler)->is_positional ()) {
                            report_unknown_option (program_name, arg_name, value, errs);
                            ok = false;
                            continue;
                        }

                        std::tie (handler, ok) =
                            record_value_if_available (handler, value, program_name, errs);
                    } else {
                        std::tie (handler, ok) = process_single_dash (arg_name, program_name, errs);
                    }
                }

                if (handler && (*handler)->takes_argument ()) {
                    errs << str::out_string (program_name) << str::out_text (": Argument '")
                         << str::out_string ((*handler)->name ())
                         << str::out_text ("' requires a value\n");
                    ok = false;
                }
                return std::make_tuple (first_arg, static_cast<bool> (ok));
            }

            template <typename InputIterator>
            bool parse_positional_arguments (InputIterator first_arg, InputIterator last_arg) {
                bool ok = true;

                auto const & all_options = option::all ();
                auto const is_positional = [] (option const * const opt) {
                    return opt->is_positional ();
                };

                auto end = std::end (all_options);
                auto it = std::find_if (std::begin (all_options), end, is_positional);
                for (; first_arg != last_arg && it != end; ++first_arg) {
                    option * const handler = *it;
                    PSTORE_ASSERT (handler->is_positional ());
                    ok = handler->add_occurrence ();
                    if (!handler->value (*first_arg)) {
                        ok = false;
                    }
                    if (!handler->can_accept_another_occurrence ()) {
                        it = std::find_if (++it, end, is_positional);
                    }
                }
                return ok;
            }

            template <typename InputIterator, typename OutputStream, typename ErrorStream>
            bool parse_command_line_options (InputIterator first_arg, InputIterator last_arg,
                                             std::string const & overview, OutputStream & outs,
                                             ErrorStream & errs) {
                std::string const program_name = pstore::path::base_name (*(first_arg++));
                help<OutputStream> const help (program_name, overview, outs, name ("help"));

                bool ok = true;
                std::tie (first_arg, ok) =
                    parse_option_arguments (first_arg, last_arg, program_name, errs);
                if (!ok) {
                    return false;
                }
                if (!parse_positional_arguments (first_arg, last_arg)) {
                    return false;
                }
                return check_for_missing (program_name, errs);
            }

        } // end namespace details

#ifdef _WIN32
        /// For Windows, a variation on the ParseCommandLineOptions functions which takes the
        /// arguments as either UTF-16 or MBCS strings and converts them to UTF-8 as expected
        /// by the rest of the code.
        template <typename CharType>
        void parse_command_line_options (int const argc, CharType * const argv[],
                                         std::string const & overview) {
            std::vector<std::string> args;
            args.reserve (argc);
            std::transform (
                argv, argv + argc, std::back_inserter (args),
                [] (CharType const * str) { return pstore::utf::from_native_string (str); });
            if (!details::parse_command_line_options (std::begin (args), std::end (args), overview,
                                                      out_stream, error_stream)) {
                std::exit (EXIT_FAILURE);
            }
        }
#else
        inline void parse_command_line_options (int const argc, gsl::zstring const argv[],
                                                std::string const & overview) {
            if (!details::parse_command_line_options (argv, argv + argc, overview, out_stream,
                                                      error_stream)) {
                std::exit (EXIT_FAILURE);
            }
        }
#endif // _WIN32

    } // end namespace command_line
} // end namespace pstore

#endif // PSTORE_COMMAND_LINE_COMMAND_LINE_HPP
