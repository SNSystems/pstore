//*      _                         _                  *
//*   __| |_   _ _ __ ___  _ __   | |_ _ __ ___  ___  *
//*  / _` | | | | '_ ` _ \| '_ \  | __| '__/ _ \/ _ \ *
//* | (_| | |_| | | | | | | |_) | | |_| | |  __/  __/ *
//*  \__,_|\__,_|_| |_| |_| .__/   \__|_|  \___|\___| *
//*                       |_|                         *
//===- tools/genromfs/dump_tree.cpp ---------------------------------------===//
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
#include "dump_tree.hpp"

// Standard Library includes
#include <ostream>

// Local includes
#include "copy.hpp"
#include "indent.hpp"
#include "vars.hpp"

namespace {

    void forward_declaration (std::ostream & os, std::unordered_set<unsigned> & forwards,
                              unsigned dirid) {
        if (forwards.find (dirid) == forwards.end ()) {
            os << "extern pstore::romfs::directory const " << directory_var (dirid) << ";\n";
            forwards.insert (dirid);
        }
    }

} // end anonymous namespace


void dump_tree (std::ostream & os, std::unordered_set<unsigned> & forwards,
                directory_container const & dir, unsigned id, unsigned parent_id) {

    for (directory_entry const & de : dir) {
        if (de.children) {
            dump_tree (os, forwards, *de.children, de.contents, id);
        }
    }
    std::string const dir_name = directory_var (id).as_string ();
    std::string const dirent_array_name = dir_name + "_membs";

    for (directory_entry const & de : dir) {
        if (de.children) {
            forward_declaration (os, forwards, de.contents);
        }
    }
    forward_declaration (os, forwards, id);
    forward_declaration (os, forwards, parent_id);

    os << "std::array<pstore::romfs::dirent," << dir.size () + 2U << "> const " << dirent_array_name
       << " = {{\n"
       << indent << "{\".\", &" << directory_var (id) << "},\n"
       << indent << "{\"..\", &" << directory_var (parent_id) << "},\n";

    for (directory_entry const & de : dir) {
        if (de.children) {
            os << indent << "{\"" << de.name << "\", &" << directory_var (de.contents);
        } else {
            auto const contents_name = file_var (de.contents);
            os << indent << "{\"" << de.name << "\", " << contents_name
               << ", pstore::romfs::stat{sizeof (" << contents_name << "), " << de.modtime
               << ", pstore::romfs::mode_t::file}";
        }
        os << "},\n";
    }

    os << "}};\n"
       << "pstore::romfs::directory const " << dir_name << " {" << dirent_array_name << "};\n";
}
