//*                                                _   _ _             *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | | (_)_ __   ___  *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | | | | '_ \ / _ \ *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | | | | | | |  __/ *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| |_|_|_| |_|\___| *
//*                                                                    *
//===- include/pstore/cmd_util/cl/command_line.hpp ------------------------===//
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
#ifndef PSTORE_CMD_UTIL_CL_COMMAND_LINE_HPP
#define PSTORE_CMD_UTIL_CL_COMMAND_LINE_HPP

#include <iostream>
#include <string>
#include <tuple>

#include "pstore/cmd_util/cl/category.hpp"
#include "pstore/cmd_util/cl/help.hpp"
#include "pstore/cmd_util/cl/modifiers.hpp"
#include "pstore/support/path.hpp"

namespace pstore {
    namespace cmd_util {
        namespace cl {

            namespace details {

                std::pair<option *, std::string>
                lookup_nearest_option (std::string const & arg,
                                       option::options_container const & all_options);

                bool starts_with (std::string const & s, char const * prefix);
                option * find_handler (std::string const & name);
                bool check_for_missing (std::string const & program_name, std::ostream & errs);


                template <typename InputIterator>
                std::tuple<InputIterator, bool>
                parse_option_arguments (InputIterator first_arg, InputIterator last_arg,
                                        std::string const & program_name, std::ostream & errs) {
                    std::string value;
                    option * handler = nullptr;
                    bool ok = true;

                    for (; first_arg != last_arg; ++first_arg) {
                        std::string arg_name = *first_arg;
                        // Is this the argument for the preceeding switch?
                        if (handler != nullptr && handler->takes_argument ()) {
                            if (!handler->value (arg_name)) {
                                ok = false;
                            }
                            handler = nullptr;
                        } else {
                            // A double-dash argument on its own indicates that the following are
                            // positional arguments.
                            if (arg_name == "--") {
                                ++first_arg; // swallow this argument.
                                break;
                            }
                            // If this argument has no leading dash, this and the following are
                            // positional arguments.
                            if (arg_name.empty () || arg_name.front () != '-') {
                                break;
                            }

                            // TODO: We treat '--' the same as '-'. Single '-' shouldn't need (or
                            // support) the '=' notation.
                            if (starts_with (arg_name, "--")) {
                                arg_name.erase (0U, 2U);
                            } else if (starts_with (arg_name, "-")) {
                                arg_name.erase (0U, 1U);
                            }

                            std::size_t const equal_pos = arg_name.find ('=');
                            if (equal_pos != std::string::npos) {
                                value = arg_name.substr (equal_pos + 1, std::string::npos);
                                arg_name = arg_name.substr (0, equal_pos);
                                // TODO: check for empty ArgName[
                            }

                            handler = find_handler (arg_name);
                            if (handler == nullptr || handler->is_positional ()) {
                                errs << program_name << ": Unknown command line argument '"
                                     << *first_arg << "'\n";

                                option * best_option = nullptr;
                                std::string nearest_string;
                                std::tie (best_option, nearest_string) =
                                    lookup_nearest_option (arg_name, option::all ());
                                if (best_option) {
                                    if (!value.empty ()) {
                                        nearest_string += '=';
                                        nearest_string += value;
                                    }
                                    errs << "Did you mean '--" << nearest_string << "'?\n";
                                }
                                ok = false;
                            } else {
                                bool const takes_argument = handler->takes_argument ();
                                bool has_value = equal_pos != std::string::npos;
                                if (takes_argument && has_value) {
                                    handler->add_occurrence ();
                                    if (!handler->value (value)) {
                                        ok = false;
                                    }
                                    handler = nullptr;
                                } else if (!takes_argument && !has_value) {
                                    handler->add_occurrence ();
                                    handler = nullptr;
                                }
                            }
                        }
                    }

                    if (handler != nullptr && handler->takes_argument ()) {
                        errs << program_name << ": Argument '" << handler->name ()
                             << "' requires a value\n";
                        ok = false;
                    }
                    return std::make_tuple (first_arg, ok);
                }

                template <typename InputIterator>
                bool parser_positional_arguments (InputIterator first_arg, InputIterator last_arg) {
                    bool ok = true;

                    auto const & all_options = option::all ();
                    auto is_positional = [](option const * const opt) {
                        return opt->is_positional ();
                    };

                    auto end = std::end (all_options);
                    auto it = std::find_if (std::begin (all_options), end, is_positional);
                    for (; first_arg != last_arg && it != end; ++first_arg) {
                        option * const handler = *it;
                        assert (handler->is_positional ());
                        handler->add_occurrence ();
                        if (!handler->value (*first_arg)) {
                            ok = false;
                        }
                        if (!handler->can_accept_another_occurrence ()) {
                            it = std::find_if (++it, end, is_positional);
                        }
                    }
                    return ok;
                }

                template <typename InputIterator>
                bool ParseCommandLineOptions (InputIterator first_arg, InputIterator last_arg,
                                              std::string const & overview,
                                              std::ostream * errs = nullptr) {

                    if (errs == nullptr) {
                        errs = &std::cerr;
                    }

                    std::string const program_name = pstore::path::base_name (*(first_arg++));
                    help help (program_name, overview, name ("help"));

                    bool ok = true;
                    std::tie (first_arg, ok) =
                        parse_option_arguments (first_arg, last_arg, program_name, *errs);

                    if (!parser_positional_arguments (first_arg, last_arg)) {
                        ok = false;
                    }

                    if (!check_for_missing (program_name, *errs)) {
                        ok = false;
                    }
                    return ok;
                }

            } // namespace details


            template <typename InputIterator>
            void ParseCommandLineOptions (InputIterator first_arg, InputIterator last_arg,
                                          std::string const & overview,
                                          std::ostream * errs = nullptr) {
                if (!details::ParseCommandLineOptions (first_arg, last_arg, overview, errs)) {
                    std::exit (EXIT_FAILURE);
                }
            }


            inline void ParseCommandLineOptions (int argc, char * argv[],
                                                 std::string const & overview,
                                                 std::ostream * errs = nullptr) {
                ParseCommandLineOptions (argv, argv + argc, overview, errs);
            }

#ifdef _WIN32
            /// For Windows, a variation on the ParseCommandLineOptions functions which takes the
            /// arguments as UTF-16 strings and converts them to UTF-8 as expected by the rest of
            /// the code.
            void ParseCommandLineOptions (int argc, wchar_t * argv[], std::string const & overview,
                                          std::ostream * errs = nullptr);
#endif
        } // namespace cl
    }     // namespace cmd_util
} // namespace pstore

#endif // PSRTORE_CMD_UTIL_CL_COMMAND_LINE_HPP
// eof: include/pstore/cmd_util/cl/command_line.hpp
