//*  _          _        *
//* | |__   ___| |_ __   *
//* | '_ \ / _ \ | '_ \  *
//* | | | |  __/ | |_) | *
//* |_| |_|\___|_| .__/  *
//*              |_|     *
//===- lib/cmd_util/help.cpp ----------------------------------------------===//
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
#include "pstore/cmd_util/help.hpp"

#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>

#include "pstore/cmd_util/word_wrapper.hpp"
#include "pstore/support/array_elements.hpp"
#include "pstore/support/ios_state.hpp"

namespace pstore {
    namespace cmd_util {
        namespace cl {

            //*  _        _       *
            //* | |_  ___| |_ __  *
            //* | ' \/ -_) | '_ \ *
            //* |_||_\___|_| .__/ *
            //*            |_|    *

            constexpr std::size_t help::overlong_opt_max;
            constexpr std::size_t help::max_width;

            // takes_argument
            // ~~~~~~~~~~~~~~
            bool help::takes_argument () const { return false; }

            // get_parser
            // ~~~~~~~~~~
            parser_base * help::get_parser () { return nullptr; }

            // value
            // ~~~~~
            bool help::value (std::string const &) { return false; }

            // max_option_length
            // ~~~~~~~~~~~~~~~~~
            int help::max_option_length () const {
                auto max_opt_len = std::size_t{0};
                for (option const * const op : cl::option::all ()) {
                    if (op != this && !op->is_alias ()) {
                        max_opt_len = std::max (max_opt_len, op->name ().length ());
                    }
                }
                return static_cast<int> (std::min (max_opt_len, overlong_opt_max));
            }

            // has_switches
            // ~~~~~~~~~~~~
            bool help::has_switches () const {
                for (option const * const op : cl::option::all ()) {
                    if (op != this && !op->is_alias () && !op->is_positional ()) {
                        return true;
                    }
                }
                return false;
            }

            // usage
            // ~~~~~
            void help::usage (std::ostream & os) const {
                os << "USAGE: " << program_name_;
                if (this->has_switches ()) {
                    os << " [options]";
                }

                for (option const * const op : cl::option::all ()) {
                    if (op != this && op->is_positional ()) {
                        os << ' ' << op->description ();
                    }
                }
            }

            // show
            // ~~~~
            void help::show (std::ostream & os) {
                static constexpr char const prefix[] = "  -";
                static constexpr char const separator[] = " - ";
                static constexpr auto const prefix_len =
                    static_cast<int> (array_elements (prefix) - 1U);
                static constexpr auto const separator_len =
                    static_cast<int> (array_elements (separator) - 1U);

                os << "OVERVIEW: " << overview_ << "\n";
                usage (os);
                os << "\n\nOPTIONS:\n";

                int const max_opt_len = max_option_length ();
                int const indent = prefix_len + max_opt_len + separator_len;

                ios_flags_saver const _{os};

                for (option const * const op : cl::option::all ()) {
                    if (op != this && !op->is_alias () && !op->is_positional ()) {
                        os << prefix << std::left << std::setw (max_opt_len) << op->name ()
                           << separator;
                        std::string const & description = op->description ();
                        auto is_first = true;
                        std::for_each (word_wrapper (description, max_width),
                                       word_wrapper::end (description, max_width),
                                       [&] (std::string const & str) {
                                           if (!is_first ||
                                               op->name ().length () > overlong_opt_max) {
                                               os << '\n' << std::setw (indent) << ' ';
                                           }
                                           os << str;
                                           is_first = false;
                                       });
                        os << '\n';
                    }
                }
            }

            // add_occurrence
            // ~~~~~~~~~~~~~~
            void help::add_occurrence () {
                this->show (std::cout);
                std::exit (EXIT_FAILURE);
            }

        } // end namespace cl
    }     // end namespace cmd_util
} // end namespace pstore
