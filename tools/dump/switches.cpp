//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/dump/switches.cpp --------------------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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

#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/revision_opt.hpp"
#include "pstore/command_line/str_to_revision.hpp"
#include "pstore/dump/digest_opt.hpp"

namespace pstore {
    namespace command_line {

        // parser<digest_opt>
        // ~~~~~~~~~~~~~~~~~~
        template <>
        class parser<dump::digest_opt> : public parser_base {
        public:
            ~parser () noexcept override = default;
            maybe<dump::digest_opt> operator() (std::string const & v) const {
                maybe<index::digest> const d = uint128::from_hex_string (v);
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

    }     // end namespace command_line
} // end namespace pstore

using namespace pstore::command_line;

namespace {

    option_category what_cat{"Options controlling what is dumped"};

    opt<bool> contents{"contents", desc{"Emit a raw dump of the transaction contents"},
                       cat (what_cat)};
    alias contents2{"c", desc{"Alias for --contents"}, aliasopt{contents}};

    list<pstore::dump::digest_opt> fragment{"fragment",
                                            desc{"Dump the contents of a specific fragment"},
                                            comma_separated, cat (what_cat)};
    alias fragment2{"F", aliasopt{fragment}};
    opt<bool> all_fragments{"all-fragments", desc{"Dump the contents of the fragments index"},
                            cat (what_cat)};

    list<pstore::dump::digest_opt> compilation{"compilation",
                                               desc{"Dump the contents of a specific compilation"},
                                               comma_separated, cat (what_cat)};
    alias compilation2{"C", aliasopt{compilation}};

    opt<bool> all_compilations{"all-compilations",
                               desc{"Dump the contents of the compilations index"}, cat (what_cat)};

    list<pstore::dump::digest_opt> debug_line_header{
        "debug-line-header", desc{"Dump the contents of a specific debug line header"},
        comma_separated, cat (what_cat)};
    opt<bool> all_debug_line_headers{"all-debug-line-headers",
                                     desc{"Dump the contents of the debug line headers index"},
                                     cat (what_cat)};


    opt<bool> header{"header", desc{"Dump the file header"}, cat (what_cat)};
    alias header2{"h", desc{"Alias for --header"}, aliasopt{header}};

    opt<bool> indices{"indices", desc{"Dump the indices"}, cat (what_cat)};
    alias indices2{"i", desc{"Alias for --indices"}, aliasopt{indices}};

    opt<bool> log_opt{"log", desc{"List the generations"}, cat (what_cat)};
    alias log2{"l", desc{"Alias for --log"}, aliasopt{log_opt}};

    opt<bool> all{
        "all",
        desc{"Show store-related output. Equivalent to: --contents --header --indices --log"},
        cat (what_cat)};
    alias all2{"a", desc{"Alias for --all"}, aliasopt{all}};

    opt<bool> shared_memory{"shared-memory", desc{"Dumps the shared-memory block"}, cat (what_cat)};
    alias shared_memory2{"s", desc{"Alias for --shared-memory"}, aliasopt{shared_memory}};


    opt<pstore::command_line::revision_opt, parser<std::string>> revision{
        "revision", desc{"The starting revision number (or 'HEAD')"}};
    alias revision2{"r", desc{"Alias for --revision"}, aliasopt{revision}};


    option_category how_cat{"Options controlling how fields are emitted"};

    opt<bool> no_times{"no-times", desc{"Times are displayed as a fixed value (for testing)"},
                       cat (how_cat)};
    opt<bool> hex{"hex", desc{"Emit number values in hexadecimal notation"}, cat (how_cat)};
    alias hex2{"x", desc{"Alias for --hex"}, aliasopt{hex}};

    opt<bool> expanded_addresses{"expanded-addresses",
                                 desc{"Emit address values as an explicit segment/offset object"},
                                 cat (how_cat)};

    opt<std::string> triple{"triple",
                            desc{"The target triple to use for disassembly if one is not known"},
                            init ("x86_64-pc-linux-gnu-repo"), cat (how_cat)};

    list<std::string> paths{positional, usage{"filename..."}};

} // end anonymous namespace

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
    parse_command_line_options (argc, argv, "pstore dump utility\n");

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
