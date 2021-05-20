//===- include/pstore/exchange/import_linked_definitions_section.hpp *- mode: C++ -*-===//
//*  _                            _     _ _       _            _  *
//* (_)_ __ ___  _ __   ___  _ __| |_  | (_)_ __ | | _____  __| | *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | | | '_ \| |/ / _ \/ _` | *
//* | | | | | | | |_) | (_) | |  | |_  | | | | | |   <  __/ (_| | *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_|_|_| |_|_|\_\___|\__,_| *
//*             |_|                                               *
//*      _       __ _       _ _   _                  *
//*   __| | ___ / _(_)_ __ (_) |_(_) ___  _ __  ___  *
//*  / _` |/ _ \ |_| | '_ \| | __| |/ _ \| '_ \/ __| *
//* | (_| |  __/  _| | | | | | |_| | (_) | | | \__ \ *
//*  \__,_|\___|_| |_|_| |_|_|\__|_|\___/|_| |_|___/ *
//*                                                  *
//*                _   _              *
//*  ___  ___  ___| |_(_) ___  _ __   *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* \__ \  __/ (__| |_| | (_) | | | | *
//* |___/\___|\___|\__|_|\___/|_| |_| *
//*                                   *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_IMPORT_LINKED_DEFINITIONS_SECTION_HPP
#define PSTORE_EXCHANGE_IMPORT_LINKED_DEFINITIONS_SECTION_HPP

#include <bitset>

#include "pstore/exchange/import_strings.hpp"
#include "pstore/exchange/import_terminals.hpp"
#include "pstore/mcrepo/linked_definitions_section.hpp"

namespace pstore {
    namespace exchange {
        namespace import_ns {

            using linked_definitions_container = std::vector<repo::linked_definitions::value_type>;

            //*  _ _      _          _      _      __ _      _ _   _              *
            //* | (_)_ _ | |_____ __| |  __| |___ / _(_)_ _ (_) |_(_)___ _ _  ___ *
            //* | | | ' \| / / -_) _` | / _` / -_)  _| | ' \| |  _| / _ \ ' \(_-< *
            //* |_|_|_||_|_\_\___\__,_| \__,_\___|_| |_|_||_|_|\__|_\___/_||_/__/ *
            //*                                                                   *
            template <typename OutputIterator>
            class linked_definition final : public rule {
            public:
                linked_definition (not_null<context *> const ctxt,
                                   not_null<OutputIterator *> const out) noexcept
                        : rule (ctxt)
                        , out_{out} {}
                linked_definition (linked_definition const &) = delete;
                linked_definition (linked_definition &&) = delete;

                ~linked_definition () override = default;

                linked_definition & operator= (linked_definition const &) = delete;
                linked_definition & operator= (linked_definition &&) = delete;

                gsl::czstring name () const noexcept override { return "linked definition"; }

                std::error_code key (std::string const & k) override;
                std::error_code end_object () override;

            private:
                not_null<OutputIterator *> const out_;

                enum { compilation, index };
                std::bitset<index + 1> seen_;

                std::string compilation_;
                std::uint64_t index_ = 0U;
            };

            // key
            // ~~~
            template <typename OutputIterator>
            std::error_code linked_definition<OutputIterator>::key (std::string const & k) {
                if (k == "compilation") {
                    seen_[compilation] = true;
                    return this->push<string_rule> (&compilation_);
                }
                if (k == "index") {
                    seen_[index] = true;
                    return this->push<uint64_rule> (&index_);
                }
                return error::unrecognized_section_object_key;
            }

            // end object
            // ~~~~~~~~~~
            template <typename OutputIterator>
            std::error_code linked_definition<OutputIterator>::end_object () {
                if (!seen_.all ()) {
                    return error::incomplete_linked_definition_object;
                }
                auto const compilation = index::digest::from_hex_string (compilation_);
                if (!compilation) {
                    return error::bad_digest;
                }
                auto const index = static_cast<std::uint32_t> (index_);
                **out_ = repo::linked_definitions::value_type (
                    *compilation, index, typed_address<repo::definition>::make (index_));
                return pop ();
            }


            //*  _ _      _          _      _      __ _      _ _   _              *
            //* | (_)_ _ | |_____ __| |  __| |___ / _(_)_ _ (_) |_(_)___ _ _  ___ *
            //* | | | ' \| / / -_) _` | / _` / -_)  _| | ' \| |  _| / _ \ ' \(_-< *
            //* |_|_|_||_|_\_\___\__,_| \__,_\___|_| |_|_||_|_|\__|_\___/_||_/__/ *
            //*                                                                   *
            //*             _   _           *
            //*  ___ ___ __| |_(_)___ _ _   *
            //* (_-</ -_) _|  _| / _ \ ' \  *
            //* /__/\___\__|\__|_\___/_||_| *
            //*                             *
            //-MARK: linked definitions section
            template <typename OutputIterator>
            class linked_definitions_section final : public rule {
            public:
                linked_definitions_section (not_null<context *> const ctxt,
                                            not_null<linked_definitions_container *> const ld,
                                            not_null<OutputIterator *> const out)
                        : rule (ctxt)
                        , ld_{ld}
                        , out_{out}
                        , links_out_{std::back_inserter (*ld)} {}

                linked_definitions_section (linked_definitions_section const &) = delete;
                linked_definitions_section (linked_definitions_section &&) noexcept = delete;

                ~linked_definitions_section () noexcept override = default;

                linked_definitions_section &
                operator= (linked_definitions_section const &) = delete;
                linked_definitions_section &
                operator= (linked_definitions_section &&) noexcept = delete;

                gsl::czstring name () const noexcept override {
                    return "linked definitions section";
                }
                std::error_code begin_object () override {
                    return this->push<linked_definition<decltype (links_out_)>> (&links_out_);
                }
                std::error_code end_array () override;

            private:
                not_null<linked_definitions_container *> const ld_;
                not_null<OutputIterator *> const out_;

                std::back_insert_iterator<linked_definitions_container> links_out_;
            };

            template <typename OutputIterator>
            std::error_code linked_definitions_section<OutputIterator>::end_array () {
                auto const * const data = ld_->data ();
                *out_ = std::make_unique<repo::linked_definitions_creation_dispatcher> (
                    data, data + ld_->size ());
                return pop ();
            }

        } // end namespace import_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_LINKED_DEFINITIONS_SECTION_HPP
