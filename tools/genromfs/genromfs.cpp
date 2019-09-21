//*                                        __      *
//*   __ _  ___ _ __  _ __ ___  _ __ ___  / _|___  *
//*  / _` |/ _ \ '_ \| '__/ _ \| '_ ` _ \| |_/ __| *
//* | (_| |  __/ | | | | | (_) | | | | | |  _\__ \ *
//*  \__, |\___|_| |_|_|  \___/|_| |_| |_|_| |___/ *
//*  |___/                                         *
//===- tools/genromfs/genromfs.cpp ----------------------------------------===//
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
#ifdef _WIN32
#    define NO_MIN_MAX
#    define WIN32_LEAN_AND_MEAN
#endif

// Standard Library includes
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

// Platform includes
#ifdef _WIN32
#    include <tchar.h>
#endif

// pstore includes
#include "pstore/cmd_util/command_line.hpp"
#include "pstore/cmd_util/tchar.hpp"
#include "pstore/support/array_elements.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/make_unique.hpp"
#include "pstore/support/portab.hpp"

// local includes
#include "copy.hpp"
#include "dump_tree.hpp"
#include "scan.hpp"
#include "vars.hpp"

namespace {

    enum class genromfs_erc : int {
        empty_name_component = 1,
    };

    class error_category : public std::error_category {
    public:
        error_category () {}
        char const * name () const noexcept { return "pstore genromfs category"; }
        std::string message (int error) const {
            static_assert (
                std::is_same<std::underlying_type<genromfs_erc>::type, decltype (error)>::value,
                "base type of genromfs_erc must be int to permit safe static cast");
            auto * result = "unknown value error";
            switch (static_cast<genromfs_erc> (error)) {
            case genromfs_erc::empty_name_component: result = "Name component is empty"; break;
            }
            return result;
        }
    };

    std::error_code make_error_code (genromfs_erc e) {
        static_assert (std::is_same<std::underlying_type<decltype (e)>::type, int>::value,
                       "base type of error_code must be int to permit safe static cast");
        static error_category const cat;
        return {static_cast<int> (e), cat};
    }

} // end anonymous namespace

namespace std {

    template <>
    struct is_error_code_enum<genromfs_erc> : std::true_type {};

} // end namespace std

#define DEFAULT_VAR "fs"

namespace {

    using namespace pstore::cmd_util;
    cl::opt<std::string> src_path (cl::positional, ".", cl::desc ("source-path"));

    cl::opt<std::string> root_var (
        "var",
        cl::desc ("Variable name for the file system root "
                  "(may contain '::' to place in a specifc namespace). (Default: '" DEFAULT_VAR
                  "')"),
        cl::init (DEFAULT_VAR));

} // end anonymous namespace

#undef DEFAULT_VAR


namespace {

    template <typename Function>
    std::string::size_type for_each_namespace (std::string const & s, Function function) {
        auto start = std::string::size_type{0};
        auto end = std::string::npos;
        static char const two_colons[] = "::";
        auto part = 0U;

        while ((end = s.find (two_colons, start)) != std::string::npos) {
            ++part;
            assert (end >= start);
            auto const length = end - start;
            if (length == 0 && part > 1U) {
                pstore::raise (genromfs_erc::empty_name_component);
            }
            if (length > 0) {
                function (s.substr (start, length));
            }
            start = end + pstore::array_elements (two_colons) - 1U;
        }
        return start;
    }

    std::ostream & write_definition (std::ostream & os, std::string const & var_name,
                                     std::string const & root) {
        auto const start = for_each_namespace (
            var_name, [&os](std::string const & ns) { os << "namespace " << ns << " {\n"; });

        auto const name = var_name.substr (start);
        if (name.length () == 0) {
            pstore::raise (genromfs_erc::empty_name_component);
        }
        os << "::pstore::romfs::romfs " << name << " (&" << root << ");\n";
        for_each_namespace (
            var_name, [&os](std::string const & ns) { os << "} // end namespace " << ns << '\n'; });
        return os;
    }

} // end anonymous namespace

#ifdef _WIN32
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    cl::ParseCommandLineOptions (argc, argv, "pstore romfs generation utility\n");
    int exit_code = EXIT_SUCCESS;

    PSTORE_TRY {
        std::ostream & os = std::cout;

        os << "// This file was generated by genromfs. DO NOT EDIT!\n"
              "#include <array>\n"
              "#include <cstdint>\n"
              "#include \"pstore/romfs/romfs.hpp\"\n"
              "\n"
              "using namespace pstore::romfs;\n"
              "namespace {\n"
              "\n";
        auto root = pstore::make_unique<directory_container> ();
        auto root_id = scan (*root, src_path.get (), 0);
        std::unordered_set<unsigned> forwards;
        dump_tree (os, forwards, *root, root_id, root_id);

        os << "\n"
              "} // end anonymous namespace\n"
              "\n";

        write_definition (os, root_var.get (), directory_var (root_id).as_string ());
    }
    PSTORE_CATCH (std::exception const & ex, {
        pstore::cmd_util::error_stream << NATIVE_TEXT ("Error: ")
                                       << pstore::utf::to_native_string (ex.what ())
                                       << NATIVE_TEXT ('\n');
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        pstore::cmd_util::error_stream << NATIVE_TEXT ("An unknown error occurred\n");
        exit_code = EXIT_FAILURE;
    })
    return exit_code;
}
