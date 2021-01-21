//*      _                         _                  *
//*   __| |_   _ _ __ ___  _ __   | |_ _ __ ___  ___  *
//*  / _` | | | | '_ ` _ \| '_ \  | __| '__/ _ \/ _ \ *
//* | (_| | |_| | | | | | | |_) | | |_| | |  __/  __/ *
//*  \__,_|\__,_|_| |_| |_| .__/   \__|_|  \___|\___| *
//*                       |_|                         *
//===- tools/genromfs/dump_tree.cpp ---------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
