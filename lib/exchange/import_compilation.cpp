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
        namespace import {

            //*     _      __ _      _ _   _           *
            //*  __| |___ / _(_)_ _ (_) |_(_)___ _ _   *
            //* / _` / -_)  _| | ' \| |  _| / _ \ ' \  *
            //* \__,_\___|_| |_|_||_|_|\__|_\___/_||_| *
            //*                                        *
            // ctor
            // ~~~~
            definition::definition (not_null<context *> ctxt, not_null<container *> definitions,
                                    not_null<name_mapping const *> names,
                                    fragment_index_pointer const & fragments)
                    : rule (ctxt)
                    , definitions_{definitions}
                    , names_{names}
                    , fragments_{fragments} {}

            // key
            // ~~~
            std::error_code definition::key (std::string const & k) {
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
                return error::unknown_definition_object_key;
            }

            // end object
            // ~~~~~~~~~~
            std::error_code definition::end_object () {
                // The visibility key is optional (defaulting to "default_vis" unsurprisingly).
                auto const visibility = seen_[visibility_index]
                                            ? decode_visibility (visibility_)
                                            : just (repo::visibility::default_vis);
                if (!visibility) {
                    return error::bad_visibility;
                }
                seen_[visibility_index] = true;

                if (!seen_.all ()) {
                    return error::definition_was_incomplete;
                }

                auto const digest = uint128::from_hex_string (digest_);
                if (!digest) {
                    return error::bad_digest;
                }
                context * const ctxt = this->get_context ();
                auto const fpos = fragments_->find (*ctxt->db, *digest);
                if (fpos == fragments_->end (*ctxt->db)) {
                    return error::no_such_fragment;
                }

                auto const linkage = decode_linkage (linkage_);
                if (!linkage) {
                    return error::bad_linkage;
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
            maybe<repo::linkage> definition::decode_linkage (std::string const & linkage) {
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
        maybe<repo::visibility> definition::decode_visibility (std::string const & visibility) {
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
        definition_object::definition_object (not_null<context *> ctxt,
                                              not_null<definition::container *> definitions,
                                              not_null<name_mapping const *> names,
                                              fragment_index_pointer const & fragments)
                : rule (ctxt)
                , definitions_{definitions}
                , names_{names}
                , fragments_{fragments} {}

        // name
        // ~~~~
        gsl::czstring definition_object::name () const noexcept { return "definition object"; }

        // begin object
        // ~~~~~~~~~~~~
        std::error_code definition_object::begin_object () {
            return this->push<definition> (definitions_, names_, fragments_);
        }

        // end array
        // ~~~~~~~~~
        std::error_code definition_object::end_array () { return this->pop (); }

        } // end namespace import
    } // end namespace exchange
} // end namespace pstore
