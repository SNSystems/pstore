//*  _                            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| *
//* | | | | | | | |_) | (_) | |  | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| *
//*             |_|                    *
//*                            _ _       _   _              *
//*   ___ ___  _ __ ___  _ __ (_) | __ _| |_(_) ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ \| | |/ _` | __| |/ _ \| '_ \  *
//* | (_| (_) | | | | | | |_) | | | (_| | |_| | (_) | | | | *
//*  \___\___/|_| |_| |_| .__/|_|_|\__,_|\__|_|\___/|_| |_| *
//*                     |_|                                 *
//===- lib/exchange/import_compilation.cpp --------------------------------===//
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
#include "pstore/exchange/import_compilation.hpp"

namespace pstore {
    namespace exchange {

        //*     _      __ _      _ _   _           *
        //*  __| |___ / _(_)_ _ (_) |_(_)___ _ _   *
        //* / _` / -_)  _| | ' \| |  _| / _ \ ' \  *
        //* \__,_\___|_| |_|_||_|_|\__|_\___/_||_| *
        //*                                        *
        // ctor
        // ~~~~
        import_definition::import_definition (parse_stack_pointer const stack,
                                              container_pointer const definitions,
                                              names_pointer const names, db_pointer const db,
                                              fragment_index_pointer const & fragments)
                : import_rule (stack)
                , definitions_{definitions}
                , names_{names}
                , db_{db}
                , fragments_{fragments}
                , seen_{} {}

        // key
        // ~~~
        std::error_code import_definition::key (std::string const & k) {
            if (k == "digest") {
                seen_[digest_index] = true;
                return push<string_rule> (&digest_);
            }
            if (k == "name") {
                seen_[name_index] = true;
                return push<uint64_rule> (&name_);
            }
            if (k == "linkage") {
                seen_[linkage_index] = true;
                return push<string_rule> (&linkage_);
            }
            if (k == "visibility") {
                seen_[visibility_index] = true;
                return push<string_rule> (&visibility_);
            }
            return import_error::unknown_definition_object_key;
        }

        // end object
        // ~~~~~~~~~~
        std::error_code import_definition::end_object () {
            // The visibility key is optional (defaulting to "default_vis" unsurprisingly).
            auto const visibility = seen_[visibility_index] ? decode_visibility (visibility_)
                                                            : just (repo::visibility::default_vis);
            if (!visibility) {
                return import_error::bad_visibility;
            }
            seen_[visibility_index] = true;

            if (!seen_.all ()) {
                return import_error::definition_was_incomplete;
            }

            auto const digest = uint128::from_hex_string (digest_);
            if (!digest) {
                return import_error::bad_digest;
            }
            auto const fpos = fragments_->find (*db_, *digest);
            if (fpos == fragments_->end (*db_)) {
                return import_error::no_such_fragment;
            }

            auto const linkage = decode_linkage (linkage_);
            if (!linkage) {
                return import_error::bad_linkage;
            }

            auto const name = names_->lookup (name_);
            if (!name) {
                return name.get_error ();
            }

            definitions_->emplace_back (*digest, fpos->second, *name, *linkage, *visibility);

            return pop ();
        }

        // decode linkage
        // ~~~~~~~~~~~~~~
        maybe<repo::linkage> import_definition::decode_linkage (std::string const & linkage) {
#define X(a)                                                                                       \
    if (linkage == #a) {                                                                           \
        return just (repo::linkage::a);                                                            \
    }
            PSTORE_REPO_LINKAGES
#undef X
            return nothing<repo::linkage> ();
        }

        // decode visibility
        // ~~~~~~~~~~~~~~~~~
        maybe<repo::visibility>
        import_definition::decode_visibility (std::string const & visibility) {
            if (visibility == "default") {
                return just (repo::visibility::default_vis);
            }
            if (visibility == "hidden") {
                return just (repo::visibility::hidden_vis);
            }
            if (visibility == "protected") {
                return just (repo::visibility::protected_vis);
            }
            return nothing<repo::visibility> ();
        }


        //*     _      __ _      _ _   _                _     _        _    *
        //*  __| |___ / _(_)_ _ (_) |_(_)___ _ _    ___| |__ (_)___ __| |_  *
        //* / _` / -_)  _| | ' \| |  _| / _ \ ' \  / _ \ '_ \| / -_) _|  _| *
        //* \__,_\___|_| |_|_||_|_|\__|_\___/_||_| \___/_.__// \___\__|\__| *
        //*                                                |__/             *
        // ctor
        // ~~~~
        import_definition_object::import_definition_object (
            parse_stack_pointer const stack, definition_container_pointer const definitions,
            names_pointer const names, database const & db,
            fragment_index_pointer const & fragments)
                : import_rule (stack)
                , definitions_{definitions}
                , names_{names}
                , db_{db}
                , fragments_{fragments} {}

        // name
        // ~~~~
        gsl::czstring import_definition_object::name () const noexcept {
            return "definition object";
        }

        // begin object
        // ~~~~~~~~~~~~
        std::error_code import_definition_object::begin_object () {
            return this->push<import_definition> (definitions_, names_, &db_, fragments_);
        }

        // end array
        // ~~~~~~~~~
        std::error_code import_definition_object::end_array () { return this->pop (); }

    } // end namespace exchange
} // end namespace pstore
