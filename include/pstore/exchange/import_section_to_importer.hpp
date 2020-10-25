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
#ifndef PSTORE_EXCHANGE_IMPORT_SECTION_TO_IMPORTER_HPP
#define PSTORE_EXCHANGE_IMPORT_SECTION_TO_IMPORTER_HPP

#include "pstore/exchange/import_bss_section.hpp"
#include "pstore/exchange/import_debug_line_section.hpp"
#include "pstore/exchange/import_generic_section.hpp"
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
    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_SECTION_TO_IMPORTER_HPP
