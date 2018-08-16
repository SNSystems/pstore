//*  _          _        *
//* | |__   ___| |_ __   *
//* | '_ \ / _ \ | '_ \  *
//* | | | |  __/ | |_) | *
//* |_| |_|\___|_| .__/  *
//*              |_|     *
//===- include/pstore/cmd_util/cl/help.hpp --------------------------------===//
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
#ifndef PSTORE_CMD_UTIL_CL_HELP_HPP
#define PSTORE_CMD_UTIL_CL_HELP_HPP

#include <limits>
#include "pstore/cmd_util/cl/option.hpp"

namespace pstore {
    namespace cmd_util {
        namespace cl {

            class help : public option {
            public:
                template <class... Mods>
                explicit help (std::string const & program_name,
                               std::string const & program_overview, Mods const &... mods)
                        : program_name_{program_name}
                        , overview_{program_overview} {

                    static_assert (max_width > overlong_opt_max,
                                   "Must allow some space for the descriptions!");
                    apply (*this, mods...);
                }

                help (help const &) = delete;
                help & operator= (help const &) = delete;

                bool takes_argument () const override;
                void add_occurrence () override;
                parser_base * get_parser () override;
                bool value (std::string const & v) override;

                void show (std::ostream & os);

            private:
                static constexpr std::size_t overlong_opt_max = 20;
                static_assert (overlong_opt_max <= std::numeric_limits<int>::max (),
                               "overlong_opt_max is too huge!");
                static constexpr std::size_t max_width = 78;

                int max_option_length () const;
                /// Returns true if the program has any non-positional arguments.
                bool has_switches () const;
                /// Writes the program's usage string to the given output stream.
                void usage (std::ostream & os) const;

                std::string const program_name_;
                std::string const overview_;
            };

        } // namespace cl
    }     // namespace cmd_util
} // namespace pstore

#endif // PSTORE_CMD_UTIL_CL_HELP_HPP
