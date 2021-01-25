//*  _          _        *
//* | |__   ___| |_ __   *
//* | '_ \ / _ \ | '_ \  *
//* | | | |  __/ | |_) | *
//* |_| |_|\___|_| .__/  *
//*              |_|     *
//===- lib/command_line/help.cpp ------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/help.hpp"

#include <iostream>
#include <iterator>
#include <string>

#ifdef _WIN32
#    include <Windows.h>
#else
#    include <sys/ioctl.h>
#    include <termios.h>
#    include <unistd.h>
#endif // _WIN32

namespace {

#ifdef _WIN32

    unsigned terminal_width () noexcept {
        HANDLE soh = ::GetStdHandle (STD_OUTPUT_HANDLE);
        if (soh == INVALID_HANDLE_VALUE) {
            return 0U;
        }
        CONSOLE_SCREEN_BUFFER_INFO csbi{};
        if (::GetConsoleScreenBufferInfo (soh, &csbi) == 0) {
            return 0U;
        }
        auto const & window = csbi.srWindow;
        if (window.Right < window.Left) {
            return 0U;
        }
        return window.Right - window.Left + 1U;
    }

#else

    unsigned terminal_width () noexcept {
        winsize w{};
        return ::ioctl (STDOUT_FILENO, TIOCGWINSZ, &w) == -1 ? 0U : w.ws_col; // NOLINT
    }

#endif //_WIN32

    auto option_string (pstore::command_line::option const & op)
        -> std::pair<std::string, std::size_t> {

        std::string const & name = op.name ();
        std::size_t code_points = pstore::utf::length (name);

        std::string str;
        if (code_points < 2U) {
            str = "-" + name;
            code_points += 1U;
        } else {
            str = "--" + name;
            code_points += 2U;
        }

        // Add the argument value's meta-name.
        pstore::gsl::czstring const desc = op.takes_argument () ? op.arg_description () : nullptr;
        if (desc != nullptr) {
            if (code_points > 2U) {
                str += "=";
                ++code_points;
            }
            str += '<';
            str += desc;
            str += '>';
            code_points += pstore::utf::length (desc) + 2;
        }
        PSTORE_ASSERT (pstore::utf::length (str) == code_points);
        return {str, code_points};
    }

} // end anonymous namespace


namespace pstore {
    namespace command_line {
            namespace details {

                bool less_name::operator() (gsl::not_null<option const *> const x,
                                            gsl::not_null<option const *> const y) const {
                    return x->name () < y->name ();
                }

                // get_max_width
                // ~~~~~~~~~~~~~
                std::size_t get_max_width () {
                    std::size_t max_width = terminal_width ();
                    if (max_width == 0U) {
                        // We couldn't figure out the terminal width, so just guess at 80.
                        max_width = 80U;
                    }
                    if (max_width < overlong_opt_max) {
                        max_width = overlong_opt_max * 2U;
                    }
                    return max_width;
                }

                // build_categories
                // ~~~~~~~~~~~~~~~~
                categories_collection build_categories (option const * const self,
                                                        option::options_container const & all) {
                    categories_collection categories;
                    for (option const * const op : all) {
                        if (op != self && !op->is_positional ()) {
                            categories[op->category ()].insert (op);
                        }
                    }
                    return categories;
                }

                // get_switch_strings
                // ~~~~~~~~~~~~~~~~~~
                auto get_switch_strings (options_set const & ops) -> switch_strings {
                    constexpr char const separator[] = ", ";
                    constexpr auto separator_len = array_elements (separator) - 1U;

                    switch_strings names;
                    for (option const * op : ops) {
                        std::pair<std::string, std::size_t> name = option_string (*op);
                        PSTORE_ASSERT (pstore::utf::length (std::get<std::string> (name)) ==
                                       std::get<std::size_t> (name));

                        if (alias const * const alias = op->as_alias ()) {
                            op = alias->original ();
                        }

                        auto & v = names[op];
                        if (!v.empty ()) {
                            auto & prev = v.back ();
                            auto & prev_string = std::get<std::string> (prev);
                            auto & prev_code_points = std::get<std::size_t> (prev);

                            if (prev_code_points + separator_len + std::get<std::size_t> (name) <=
                                overlong_opt_max) {
                                // Fold this string onto the same output line as its predecessor.
                                prev_string += separator + std::get<std::string> (name);
                                prev_code_points += separator_len + std::get<std::size_t> (name);

                                PSTORE_ASSERT (pstore::utf::length (prev_string) ==
                                               prev_code_points);
                                continue;
                            }
                        }
                        v.push_back (name);
                    }
                    return names;
                }

                // widest_option
                // ~~~~~~~~~~~~~
                std::size_t widest_option (categories_collection const & categories) {
                    auto max_name_len = std::size_t{0};
                    for (auto const & cat : categories) {
                        for (auto const & sw : get_switch_strings (cat.second)) {
                            for (std::tuple<std::string, std::size_t> const & name : sw.second) {
                                max_name_len =
                                    std::max (max_name_len, std::get<std::size_t> (name));
                            }
                        }
                    }
                    return std::min (max_name_len, overlong_opt_max);
                }

                // has_switches
                // ~~~~~~~~~~~~
                bool has_switches (option const * const self,
                                   option::options_container const & all) {
                    return std::any_of (
                        std::begin (all), std::end (all), [self] (option const * const op) {
                            return op != self && !op->is_alias () && !op->is_positional ();
                        });
                }

            } // end namespace details
    }         // end namespace command_line
} // end namespace pstore
