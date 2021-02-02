//*  _                            _                   _   _               _         *
//* (_)_ __ ___  _ __   ___  _ __| |_   ___  ___  ___| |_(_) ___  _ __   | |_ ___   *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| / __|/ _ \/ __| __| |/ _ \| '_ \  | __/ _ \  *
//* | | | | | | | |_) | (_) | |  | |_  \__ \  __/ (__| |_| | (_) | | | | | || (_) | *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |___/\___|\___|\__|_|\___/|_| |_|  \__\___/  *
//*             |_|                                                                 *
//*  _                            _             *
//* (_)_ __ ___  _ __   ___  _ __| |_ ___ _ __  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __/ _ \ '__| *
//* | | | | | | | |_) | (_) | |  | ||  __/ |    *
//* |_|_| |_| |_| .__/ \___/|_|   \__\___|_|    *
//*             |_|                             *
//===- include/pstore/exchange/import_section_to_importer.hpp -------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_IMPORT_SECTION_TO_IMPORTER_HPP
#define PSTORE_EXCHANGE_IMPORT_SECTION_TO_IMPORTER_HPP

#include "pstore/exchange/import_bss_section.hpp"
#include "pstore/exchange/import_debug_line_section.hpp"
#include "pstore/exchange/import_linked_definitions_section.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            /// We can map from the section_kind enum to the type of data used to represent a
            /// section of that kind. section_to_importer is used to convert from this type class
            /// that we use to import that data. For example, the text section is represented by the
            /// generic_section type. We then use the import_generic_section class to import it.
            /// There are template specializations for each of the section content types in the
            /// database.
            template <typename Section, typename OutputIterator>
            struct section_to_importer {};
            template <typename Section, typename OutputIterator>
            using section_to_importer_t =
                typename section_to_importer<Section, OutputIterator>::type;

            template <typename OutputIterator>
            struct section_to_importer<repo::generic_section, OutputIterator> {
                using type = generic_section<OutputIterator>;
            };
            template <typename OutputIterator>
            struct section_to_importer<repo::bss_section, OutputIterator> {
                using type = bss_section<OutputIterator>;
            };
            template <typename OutputIterator>
            struct section_to_importer<repo::linked_definitions, OutputIterator> {
                using type = linked_definitions_section<OutputIterator>;
            };
            template <typename OutputIterator>
            struct section_to_importer<repo::debug_line_section, OutputIterator> {
                using type = debug_line_section<OutputIterator>;
            };

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_SECTION_TO_IMPORTER_HPP
