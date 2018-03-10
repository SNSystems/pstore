//*                 _              *
//*  _ __ ___  __ _(_) ___  _ __   *
//* | '__/ _ \/ _` | |/ _ \| '_ \  *
//* | | |  __/ (_| | | (_) | | | | *
//* |_|  \___|\__, |_|\___/|_| |_| *
//*           |___/                *
//===- lib/core/region.cpp ------------------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
/// \file region.cpp
/// \brief A factory for memory-mapper objects which is used for both the initial file
///        allocation and to grow files as allocations are performed in a transaction.
/// The "real" library exclusively uses the file-based factory; the memory-based
/// factory is used for unit testing.

#include "pstore/core/region.hpp"

#include "pstore/config/config.hpp"
#include "pstore/core/make_unique.hpp"

namespace pstore {
    namespace region {

        // small_files_enabled
        // ~~~~~~~~~~~~~~~~~~~
        bool small_files_enabled () {
#if PSTORE_POSIX_SMALL_FILES
            return true;
#else
            return false;
#endif
        }

        std::unique_ptr<factory> get_factory (std::shared_ptr<file::file_handle> file,
                                              std::uint64_t full_size, std::uint64_t min_size) {
            return std::make_unique<file_based_factory> (file, full_size, min_size);
        }

        std::unique_ptr<factory> get_factory (std::shared_ptr<file::in_memory> file,
                                              std::uint64_t full_size, std::uint64_t min_size) {
            return std::make_unique<mem_based_factory> (file, full_size, min_size);
        }


        /*   __ _ _        _                        _    __            _                    *
         *  / _(_) | ___  | |__   __ _ ___  ___  __| |  / _| __ _  ___| |_ ___  _ __ _   _  *
         * | |_| | |/ _ \ | '_ \ / _` / __|/ _ \/ _` | | |_ / _` |/ __| __/ _ \| '__| | | | *
         * |  _| | |  __/ | |_) | (_| \__ \  __/ (_| | |  _| (_| | (__| || (_) | |  | |_| | *
         * |_| |_|_|\___| |_.__/ \__,_|___/\___|\__,_| |_|  \__,_|\___|\__\___/|_|   \__, | *
         *                                                                           |___/  *
         */
        // (ctor)
        // ~~~~~~
        file_based_factory::file_based_factory (std::shared_ptr<file::file_handle> file,
                                                std::uint64_t full_size, std::uint64_t min_size)
                : factory{full_size, min_size}
                , file_{std::move (file)} {}

        // init
        // ~~~~
        auto file_based_factory::init () -> std::vector<memory_mapper_ptr> {
            return this->create<file::file_handle, memory_mapper> (file_);
        }

        // add
        // ~~~
        void file_based_factory::add (std::vector<memory_mapper_ptr> * const regions,
                                      std::uint64_t original_size, std::uint64_t new_size) {

            this->append<file::file_handle, memory_mapper> (file_, regions, original_size,
                                                            new_size);
        }

        // file
        // ~~~~
        std::shared_ptr<file::file_base> file_based_factory::file () {
            return std::static_pointer_cast<file::file_base> (file_);
        }


        /*                             _                        _    __            _ * _ __ ___
         * ___ _ __ ___   | |__   __ _ ___  ___  __| |  / _| __ _  ___| |_ ___  _ __ _   _  * | '_ `
         * _ \ / _ \ '_ ` _ \  | '_ \ / _` / __|/ _ \/ _` | | |_ / _` |/ __| __/ _ \| '__| | | | *
         * | | | | | |  __/ | | | | | | |_) | (_| \__ \  __/ (_| | |  _| (_| | (__| || (_) | |  |
         * |_| | *
         * |_| |_| |_|\___|_| |_| |_| |_.__/ \__,_|___/\___|\__,_| |_|  \__,_|\___|\__\___/|_|
         * \__, | *
         *                                                                                       |___/
         * *
         */
        // (ctor)
        // ~~~~~~
        mem_based_factory::mem_based_factory (std::shared_ptr<file::in_memory> file,
                                              std::uint64_t full_size, std::uint64_t min_size)
                : factory{full_size, min_size}
                , file_{std::move (file)} {}

        // init
        // ~~~~
        auto mem_based_factory::init () -> std::vector<memory_mapper_ptr> {
            return this->create<file::in_memory, in_memory_mapper> (file_);
        }

        // add
        // ~~~
        void mem_based_factory::add (std::vector<memory_mapper_ptr> * regions,
                                     std::uint64_t original_size, std::uint64_t new_size) {

            this->append<file::in_memory, in_memory_mapper> (file_, regions, original_size,
                                                             new_size);
        }

        // file
        // ~~~~
        std::shared_ptr<file::file_base> mem_based_factory::file () {
            return std::static_pointer_cast<file::file_base> (file_);
        }

    } // namespace region
} // namespace pstore
// eof: lib/core/region.cpp
