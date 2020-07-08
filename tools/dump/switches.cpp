//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/dump/switches.cpp --------------------------------------------===//
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

#include "switches.hpp"

#include <iterator>

#include "pstore/cmd_util/command_line.hpp"
#include "pstore/cmd_util/revision_opt.hpp"
#include "pstore/cmd_util/str_to_revision.hpp"
#include "pstore/dump/digest_opt.hpp"

namespace pstore {
    namespace cmd_util {
        namespace cl {

            // parser<digest_opt>
            // ~~~~~~~~~~~~~~~~~~
            template <>
            class parser<dump::digest_opt> : public parser_base {
            public:
                ~parser () noexcept override = default;
                maybe<dump::digest_opt> operator() (std::string const & v) const {
                    maybe<index::digest> const d = dump::digest_from_string (v);
                    return d ? just (dump::digest_opt{*d}) : nothing<dump::digest_opt> ();
                }
            };

            // type_description<digest_opt>
            // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            template <>
            struct type_description<dump::digest_opt> {
                static gsl::czstring value;
            };

            gsl::czstring type_description<dump::digest_opt>::value = "digest";

        } // end namespace cl
    }     // end namespace cmd_util
} // end namespace pstore

using namespace pstore::cmd_util;

namespace {

    cl::option_category what_cat{"Options controlling what is dumped"};

    cl::opt<bool> contents{"contents", cl::desc{"Emit a raw dump of the transaction contents"},
                           cl::cat (what_cat)};
    cl::alias contents2{"c", cl::desc{"Alias for --contents"}, cl::aliasopt{contents}};

    cl::list<pstore::dump::digest_opt> fragment{
        "fragment", cl::desc{"Dump the contents of a specific fragment"}, cl::cat (what_cat)};
    cl::alias fragment2{"F", cl::aliasopt{fragment}};
    cl::opt<bool> all_fragments{
        "all-fragments", cl::desc{"Dump the contents of the fragments index"}, cl::cat (what_cat)};

    cl::list<pstore::dump::digest_opt> compilation{
        "compilation", cl::desc{"Dump the contents of a specific compilation"}, cl::cat (what_cat)};
    cl::alias compilation2{"C", cl::aliasopt{compilation}};

    cl::opt<bool> all_compilations{"all-compilations",
                                   cl::desc{"Dump the contents of the compilations index"},
                                   cl::cat (what_cat)};

    cl::list<pstore::dump::digest_opt> debug_line_header{
        "debug-line-header", cl::desc{"Dump the contents of a specific debug line header"},
        cl::cat (what_cat)};
    cl::opt<bool> all_debug_line_headers{
        "all-debug-line-headers", cl::desc{"Dump the contents of the debug line headers index"},
        cl::cat (what_cat)};


    cl::opt<bool> header{"header", cl::desc{"Dump the file header"}, cl::cat (what_cat)};
    cl::alias header2{"h", cl::desc{"Alias for --header"}, cl::aliasopt{header}};

    cl::opt<bool> indices{"indices", cl::desc{"Dump the indices"}, cl::cat (what_cat)};
    cl::alias indices2{"i", cl::desc{"Alias for --indices"}, cl::aliasopt{indices}};

    cl::opt<bool> log_opt{"log", cl::desc{"List the generations"}, cl::cat (what_cat)};
    cl::alias log2{"l", cl::desc{"Alias for --log"}, cl::aliasopt{log_opt}};

    cl::opt<bool> all{
        "all",
        cl::desc{"Show store-related output. Equivalent to: --contents --header --indices --log"},
        cl::cat (what_cat)};
    cl::alias all2{"a", cl::desc{"Alias for --all"}, cl::aliasopt{all}};

    cl::opt<bool> shared_memory{"shared-memory", cl::desc{"Dumps the shared-memory block"},
                                cl::cat (what_cat)};
    cl::alias shared_memory2{"s", cl::desc{"Alias for --shared-memory"},
                             cl::aliasopt{shared_memory}};


    cl::opt<pstore::cmd_util::revision_opt, cl::parser<std::string>> revision{
        "revision", cl::desc{"The starting revision number (or 'HEAD')"}};
    cl::alias revision2{"r", cl::desc{"Alias for --revision"}, cl::aliasopt{revision}};


    cl::option_category how_cat{"Options controlling how fields are emitted"};

    cl::opt<bool> no_times{"no-times",
                           cl::desc{"Times are displayed as a fixed value (for testing)"},
                           cl::cat (how_cat)};
    cl::opt<bool> hex{"hex", cl::desc{"Emit number values in hexadecimal notation"},
                      cl::cat (how_cat)};
    cl::alias hex2{"x", cl::desc{"Alias for --hex"}, cl::aliasopt{hex}};

    cl::opt<bool> expanded_addresses{
        "expanded-addresses", cl::desc{"Emit address values as an explicit segment/offset object"},
        cl::cat (how_cat)};

    cl::opt<std::string> triple{
        "triple", cl::desc{"The target triple to use for disassembly if one is not known"},
        cl::init ("x86_64-pc-linux-gnu-repo"), cl::cat (how_cat)};

    cl::list<std::string> paths{cl::positional, cl::desc{"<filename>..."}};

} // end anonymous namespace

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
    cl::parse_command_line_options (argc, argv, "pstore dump utility\n");

    switches result;
    result.show_contents = contents.get ();

    auto const get_digest_from_opt = [] (pstore::dump::digest_opt const & d) {
        return pstore::index::digest{d};
    };
    std::transform (std::begin (fragment), std::end (fragment),
                    std::back_inserter (result.fragments), get_digest_from_opt);
    result.show_all_fragments = all_fragments.get ();

    std::transform (std::begin (compilation), std::end (compilation),
                    std::back_inserter (result.compilations), get_digest_from_opt);
    result.show_all_compilations = all_compilations.get ();

    std::transform (std::begin (debug_line_header), std::end (debug_line_header),
                    std::back_inserter (result.debug_line_headers), get_digest_from_opt);
    result.show_all_debug_line_headers = all_debug_line_headers.get ();

    result.show_header = header.get ();
    result.show_indices = indices.get ();
    result.show_log = log_opt.get ();
    result.show_shared = shared_memory.get ();
    result.show_all = all.get ();
    result.revision = static_cast<unsigned> (revision.get ());

    result.hex = hex.get ();
    result.no_times = no_times.get ();
    result.expanded_addresses = expanded_addresses.get ();
    result.triple = triple.get ();
    result.paths = paths.get ();

    return {result, EXIT_SUCCESS};
}
