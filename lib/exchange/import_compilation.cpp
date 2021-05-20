//===- lib/exchange/import_compilation.cpp --------------------------------===//
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
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_compilation.hpp"

namespace pstore {
    namespace exchange {
        namespace import_ns {

            //*     _      __ _      _ _   _           *
            //*  __| |___ / _(_)_ _ (_) |_(_)___ _ _   *
            //* / _` / -_)  _| | ' \| |  _| / _ \ ' \  *
            //* \__,_\___|_| |_|_||_|_|\__|_\___/_||_| *
            //*                                        *
            // ctor
            // ~~~~
            definition::definition (not_null<context *> const ctxt,
                                    not_null<container *> const definitions,
                                    not_null<string_mapping const *> const names,
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
            //-MARK: definition object
            // (ctor)
            // ~~~~~~
            definition_object::definition_object (
                not_null<context *> const ctxt, not_null<definition::container *> const definitions,
                not_null<string_mapping const *> const names,
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

            //*                    _ _      _   _           *
            //*  __ ___ _ __  _ __(_) |__ _| |_(_)___ _ _   *
            //* / _/ _ \ '  \| '_ \ | / _` |  _| / _ \ ' \  *
            //* \__\___/_|_|_| .__/_|_\__,_|\__|_\___/_||_| *
            //*              |_|                            *
            //-MARK: compilation
            // (ctor)
            // ~~~~~~
            compilation::compilation (not_null<context *> const ctxt,
                                      not_null<transaction_base *> const transaction,
                                      not_null<string_mapping const *> const names,
                                      fragment_index_pointer const & fragments,
                                      index::digest const & digest)
                    : rule (ctxt)
                    , transaction_{transaction}
                    , names_{names}
                    , fragments_{fragments}
                    , digest_{digest} {}

            // key
            // ~~~
            std::error_code compilation::key (std::string const & k) {
                if (k == "triple") {
                    seen_[triple_index] = true;
                    return push<uint64_rule> (&triple_);
                }
                if (k == "definitions") {
                    seen_[definitions_index] = true;
                    return push_array_rule<definition_object> (this, &definitions_, names_,
                                                               fragments_);
                }
                return error::unknown_compilation_object_key;
            }

            // end object
            // ~~~~~~~~~~
            std::error_code compilation::end_object () {
                if (!seen_.all ()) {
                    return error::incomplete_compilation_object;
                }
                auto const triple = names_->lookup (triple_);
                if (!triple) {
                    return triple.get_error ();
                }

                // Create the compilation record in the store.
                extent<repo::compilation> const compilation_extent = repo::compilation::alloc (
                    *transaction_, *triple, std::begin (definitions_), std::end (definitions_));

                // Insert this compilation into the compilations index.
                auto const compilations =
                    pstore::index::get_index<pstore::trailer::indices::compilation> (
                        transaction_->db ());
                compilations->insert (*transaction_, std::make_pair (digest_, compilation_extent));

                return pop ();
            }

            //*                    _ _      _   _               _         _          *
            //*  __ ___ _ __  _ __(_) |__ _| |_(_)___ _ _  ___ (_)_ _  __| |_____ __ *
            //* / _/ _ \ '  \| '_ \ | / _` |  _| / _ \ ' \(_-< | | ' \/ _` / -_) \ / *
            //* \__\___/_|_|_| .__/_|_\__,_|\__|_\___/_||_/__/ |_|_||_\__,_\___/_\_\ *
            //*              |_|                                                     *
            //-MARK: compilations index
            // (ctor)
            // ~~~~~~
            compilations_index::compilations_index (not_null<context *> const ctxt,
                                                    not_null<transaction_base *> const transaction,
                                                    not_null<string_mapping const *> const names)
                    : rule (ctxt)
                    , transaction_{transaction}
                    , names_{names}
                    , fragments_{
                          index::get_index<trailer::indices::fragment> (transaction->db ())} {}

            // name
            // ~~~~
            gsl::czstring compilations_index::name () const noexcept {
                return "compilations index";
            }

            // key
            // ~~~
            std::error_code compilations_index::key (std::string const & s) {
                if (maybe<index::digest> const digest = uint128::from_hex_string (s)) {
                    return push_object_rule<compilation> (this, transaction_, names_, fragments_,
                                                          index::digest{*digest});
                }
                return error::bad_digest;
            }

            // end object
            // ~~~~~~~~~~
            std::error_code compilations_index::end_object () { return pop (); }


        } // end namespace import_ns
    }     // end namespace exchange
} // end namespace pstore
