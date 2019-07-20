//*                                                _   _ _             *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | | (_)_ __   ___  *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | | | | '_ \ / _ \ *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | | | | | | |  __/ *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| |_|_|_| |_|\___| *
//*                                                                    *
//===- lib/cmd_util/command_line.cpp --------------------------------------===//
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
#include "pstore/cmd_util/command_line.hpp"

#include <iostream>

#include "pstore/cmd_util/option.hpp"
#include "pstore/cmd_util/string_distance.hpp"
#include "pstore/support/utf.hpp"

namespace pstore {
    namespace cmd_util {
        namespace cl {
            namespace details {

                // lookup_nearest_option
                // ~~~~~~~~~~~~~~~~~~~~~
                std::pair<option *, std::string>
                lookup_nearest_option (std::string const & arg,
                                       option::options_container const & all_options) {
                    if (arg.empty ()) {
                        return {nullptr, std::string{}};
                    }

                    // Find the closest match.
                    option * best_option = nullptr;
                    std::string nearest_string;
                    auto best_distance = std::string::size_type{0};
                    for (auto const & opt : all_options) {
                        auto const name = opt->name ();
                        auto const distance = string_distance (name, arg, best_distance);
                        if (best_option == nullptr || distance < best_distance) {
                            best_option = opt;
                            best_distance = distance;
                            nearest_string = name;
                        }
                    }

                    return {best_option, nearest_string};
                }


                bool starts_with (std::string const & s, char const * prefix) {
                    auto it = std::begin (s);
                    auto end = std::end (s);
                    for (; *prefix != '\0' && it != end; ++it, ++prefix) {
                        if (*it != *prefix) {
                            return false;
                        }
                    }
                    return *prefix == '\0';
                }

                pstore::cmd_util::cl::option * find_handler (std::string const & name) {
                    using pstore::cmd_util::cl::option;

                    auto const & all_options = option::all ();
                    auto predicate = [&name](option * const opt) { return opt->name () == name; };
                    auto end = std::end (all_options);
                    auto it = std::find_if (std::begin (all_options), end, predicate);
                    return it != end ? *it : nullptr;
                }

                // Make sure all of the required args have been specified.
                bool check_for_missing (std::string const & program_name, std::ostream & errs) {
                    using pstore::cmd_util::cl::num_occurrences_flag;
                    using pstore::cmd_util::cl::option;

                    bool ok = true;
                    auto positional_missing = 0U;

                    for (option const * opt : option::all ()) {
                        switch (opt->get_num_occurrences_flag ()) {
                        case num_occurrences_flag::required:
                        case num_occurrences_flag::one_or_more:
                            if (opt->getNumOccurrences () == 0U) {
                                if (opt->is_positional ()) {
                                    ++positional_missing;
                                } else {
                                    errs << program_name << ": option '" << opt->name ()
                                         << "' must be specified at least once\n";
                                }
                                ok = false;
                            }
                            break;
                        case num_occurrences_flag::optional:
                        case num_occurrences_flag::zero_or_more: break;
                        }
                    }

                    if (positional_missing != 0U) {
                        errs << program_name << ": ";
                        if (positional_missing == 1U) {
                            errs << "a positional argument was missing";
                        } else if (positional_missing > 1U) {
                            errs << positional_missing << " positional arguments are missing";
                        }
                        errs << '\n';
                    }

                    return ok;
                }

            } // namespace details


#ifdef _WIN32
            void ParseCommandLineOptions (int argc, wchar_t * argv[], std::string const & overview,
                                          std::ostream * errs) {
                std::vector<std::string> args;
                args.reserve (argc);
                std::transform (
                    argv, argv + argc, std::back_inserter (args),
                    [](wchar_t const * str) { return pstore::utf::from_native_string (str); });
                cl::ParseCommandLineOptions (std::begin (args), std::end (args), overview, errs);
            }
#endif

        } // namespace cl
    }     // namespace cmd_util
} // namespace pstore
