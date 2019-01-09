//*      _                         _                  *
//*   __| |_   _ _ __ ___  _ __   | |_ _ __ ___  ___  *
//*  / _` | | | | '_ ` _ \| '_ \  | __| '__/ _ \/ _ \ *
//* | (_| | |_| | | | | | | |_) | | |_| | |  __/  __/ *
//*  \__,_|\__,_|_| |_| |_| .__/   \__|_|  \___|\___| *
//*                       |_|                         *
//===- tools/genromfs/dump_tree.cpp ---------------------------------------===//
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
#include "dump_tree.hpp"

#include <ostream>
#include "./indent.hpp"

namespace {

    void forward_declaration (std::ostream & os, std::unordered_set<unsigned> & forwards,
                              unsigned dirid, unsigned id) {
        if (dirid >= id && forwards.find (dirid) == forwards.end ()) {
            os << "extern pstore::romfs::directory const dir" << dirid << ";\n";
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
    std::string const dir_name = std::string{"dir"} + std::to_string (id);
    std::string const dirent_array_name = dir_name + "_membs";

    for (directory_entry const & de : dir) {
        if (de.children) {
            forward_declaration (os, forwards, de.contents, id);
        }
    }
    forward_declaration (os, forwards, id, id);
    forward_declaration (os, forwards, parent_id, id);

    os << "std::array<pstore::romfs::dirent," << dir.size () + 2U << "> const " << dirent_array_name
       << " = {{\n"
       << indent << "{\".\", &dir" << id << "},\n"
       << indent << "{\"..\", &dir" << parent_id << "},\n";

    for (directory_entry const & de : dir) {
        if (de.children) {
            os << indent << "{\"" << de.name << "\", &dir" << de.contents;
        } else {
            auto const contents_name = "file" + std::to_string (de.contents);
            os << indent << "{\"" << de.name << "\", " << contents_name << ", sizeof ("
               << contents_name << "), 0";
        }
        os << "},\n";
    }

    os << "}};\n"
       << "pstore::romfs::directory const " << dir_name << " {" << dirent_array_name << "};\n";
}
