//*                                                _   _ _             *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | | (_)_ __   ___  *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | | | | '_ \ / _ \ *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | | | | | | |  __/ *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| |_|_|_| |_|\___| *
//*                                                                    *
//===- lib/cmd_util/command_line.cpp --------------------------------------===//
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
#include "pstore/cmd_util/command_line.hpp"

#include "pstore/cmd_util/string_distance.hpp"

namespace pstore {
    namespace cmd_util {
        namespace cl {
            namespace details {

                // lookup_nearest_option
                // ~~~~~~~~~~~~~~~~~~~~~
                maybe<option *>
                lookup_nearest_option (std::string const & arg,
                                       option::options_container const & all_options) {
                    auto best_option = nothing<option *> ();
                    if (arg.empty ()) {
                        return best_option;
                    }
                    // Find the closest match.
                    auto best_distance = std::numeric_limits<std::string::size_type>::max ();
                    for (auto const & opt : all_options) {
                        auto const distance = string_distance (opt->name (), arg, best_distance);
                        if (distance < best_distance) {
                            best_option = opt;
                            best_distance = distance;
                        }
                    }
                    return best_option;
                }

                // starts_with
                // ~~~~~~~~~~~
                bool starts_with (std::string const & s, gsl::czstring prefix) {
                    auto const end = std::end (s);
                    for (auto it = std::begin (s); *prefix != '\0' && it != end; ++it, ++prefix) {
                        if (*it != *prefix) {
                            return false;
                        }
                    }
                    return *prefix == '\0';
                }

                // find_handler
                // ~~~~~~~~~~~~
                maybe<option *> find_handler (std::string const & name) {
                    using pstore::cmd_util::cl::option;

                    auto const & all_options = option::all ();
                    auto const end = std::end (all_options);
                    auto const it =
                        std::find_if (std::begin (all_options), end, [&name] (option * const opt) {
                            return opt->name () == name;
                        });
                    return it != end ? just (*it) : nothing<option *> ();
                }

                // argument_is_positional
                // ~~~~~~~~~~~~~~~~~~~~~~
                bool argument_is_positional (std::string const & arg_name) {
                    return arg_name.empty () || arg_name.front () != '-';
                }

                // handler_takes_argument
                // ~~~~~~~~~~~~~~~~~~~~~~
                bool handler_takes_argument (maybe<option *> handler) {
                    return handler && (*handler)->takes_argument ();
                }

                // handler_set_value
                // ~~~~~~~~~~~~~~~~~
                bool handler_set_value (maybe<option *> handler, std::string const & value) {
                    assert (handler_takes_argument (handler));
                    if (!(*handler)->add_occurrence ()) {
                        return false;
                    }
                    return (*handler)->value (value);
                }

                // get_option_and_value
                // ~~~~~~~~~~~~~~~~~~~~
                std::tuple<std::string, maybe<std::string>> get_option_and_value (std::string arg) {
                    static constexpr char double_dash[] = "--";
                    static constexpr auto double_dash_len = std::string::size_type{2};

                    auto value = nothing<std::string> ();
                    if (starts_with (arg, double_dash)) {
                        std::size_t const equal_pos = arg.find ('=', double_dash_len);
                        if (equal_pos == std::string::npos) {
                            arg.erase (0U, double_dash_len);
                        } else {
                            value = just (arg.substr (equal_pos + 1, std::string::npos));
                            assert (equal_pos >= double_dash_len);
                            arg = arg.substr (double_dash_len, equal_pos - double_dash_len);
                        }
                    } else {
                        assert (starts_with (arg, "-"));
                        arg.erase (0U, 1U);
                    }
                    return std::make_tuple (arg, value);
                }

            } // end namespace details
        }     // end namespace cl
    }         // end namespace cmd_util
} // end namespace pstore
