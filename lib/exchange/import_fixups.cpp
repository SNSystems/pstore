//===- lib/exchange/import_fixups.cpp -------------------------------------===//
//*  _                            _      __ _                       *
//* (_)_ __ ___  _ __   ___  _ __| |_   / _(_)_  ___   _ _ __  ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | |_| \ \/ / | | | '_ \/ __| *
//* | | | | | | | |_) | (_) | |  | |_  |  _| |>  <| |_| | |_) \__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_| |_/_/\_\\__,_| .__/|___/ *
//*             |_|                                     |_|         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file import_fixups.cpp
/// \brief  Implements the rules for importing internal- and external-fixup records.
#include "pstore/exchange/import_fixups.hpp"

namespace pstore {
    namespace exchange {
        namespace import_ns {

            namespace details {

                // string value
                // ~~~~~~~~~~~~
                std::error_code section_name::string_value (std::string const & s) {
                    // TODO: this map appears both here and in the fragment code.
#define X(a) {#a, pstore::repo::section_kind::a},
                    static std::unordered_map<std::string, repo::section_kind> map = {
                        PSTORE_MCREPO_SECTION_KINDS};
#undef X
                    auto const pos = map.find (s);
                    if (pos == map.end ()) {
                        return error::unknown_section_name;
                    }
                    *section_ = pos->second;
                    return pop ();
                }

            } // end namespace details

            //*  _     _                     _    __ _                *
            //* (_)_ _| |_ ___ _ _ _ _  __ _| |  / _(_)_ ___  _ _ __  *
            //* | | ' \  _/ -_) '_| ' \/ _` | | |  _| \ \ / || | '_ \ *
            //* |_|_||_\__\___|_| |_||_\__,_|_| |_| |_/_\_\\_,_| .__/ *
            //*                                                |_|    *
            //-MARK:internal fixup
            // (ctor)
            // ~~~~~~
            internal_fixup::internal_fixup (not_null<context *> const ctxt,
                                            not_null<string_mapping const *> const /*names*/,
                                            fixups_pointer const fixups)
                    : rule (ctxt)
                    , fixups_{fixups} {}

            // name
            // ~~~~
            gsl::czstring internal_fixup::name () const noexcept { return "internal fixup"; }

            // key
            // ~~~
            std::error_code internal_fixup::key (std::string const & k) {
                if (k == "section") {
                    seen_[section] = true;
                    return this->push<details::section_name> (&section_);
                }
                if (k == "type") {
                    seen_[type] = true;
                    return this->push<uint64_rule> (&type_);
                }
                if (k == "offset") {
                    seen_[offset] = true;
                    return this->push<uint64_rule> (&offset_);
                }
                if (k == "addend") {
                    seen_[addend] = true;
                    return this->push<int64_rule> (&addend_);
                }
                return error::unrecognized_ifixup_key;
            }

            // end object
            // ~~~~~~~~~~
            std::error_code internal_fixup::end_object () {
                if (!seen_.all ()) {
                    return error::ifixup_object_was_incomplete;
                }
                // TODO: validate more values here.
                fixups_->emplace_back (section_, static_cast<pstore::repo::relocation_type> (type_),
                                       offset_, addend_);
                return pop ();
            }

            //*          _                     _    __ _                *
            //*  _____ _| |_ ___ _ _ _ _  __ _| |  / _(_)_ ___  _ _ __  *
            //* / -_) \ /  _/ -_) '_| ' \/ _` | | |  _| \ \ / || | '_ \ *
            //* \___/_\_\\__\___|_| |_||_\__,_|_| |_| |_/_\_\\_,_| .__/ *
            //*                                                  |_|    *
            //-MARK:external fixup
            // (ctor)
            // ~~~~~~
            external_fixup::external_fixup (not_null<context *> const ctxt,
                                            not_null<string_mapping const *> const names,
                                            fixups_pointer const fixups)
                    : rule (ctxt)
                    , names_{names}
                    , fixups_{fixups} {
                seen_[is_weak] = true; // 'is_weak' defaults to false if not present in the input.
            }

            // name
            // ~~~~~
            gsl::czstring external_fixup::name () const noexcept { return "external fixup"; }

            // key
            // ~~~
            std::error_code external_fixup::key (std::string const & k) {
                if (k == "name") {
                    seen_[name_index] = true;
                    return this->push<uint64_rule> (&name_);
                }
                if (k == "type") {
                    seen_[type] = true;
                    return this->push<uint64_rule> (&type_);
                }
                if (k == "is_weak") {
                    seen_[is_weak] = true;
                    return this->push<bool_rule> (&is_weak_);
                }
                if (k == "offset") {
                    seen_[offset] = true;
                    return this->push<uint64_rule> (&offset_);
                }
                if (k == "addend") {
                    seen_[addend] = true;
                    return this->push<int64_rule> (&addend_);
                }
                return error::unrecognized_xfixup_key;
            }

            // end object
            // ~~~~~~~~~~
            std::error_code external_fixup::end_object () {
                if (!seen_.all ()) {
                    return error::xfixup_object_was_incomplete;
                }

                auto name = names_->lookup (name_);
                if (!name) {
                    return name.get_error ();
                }

                // TODO: validate some values here.
                fixups_->emplace_back (*name, static_cast<repo::relocation_type> (type_),
                                       is_weak_ ? repo::binding::weak : repo::binding::strong,
                                       offset_, addend_);
                return pop ();
            }

        } // end namespace import_ns
    }     // end namespace exchange
} // end namespace pstore
