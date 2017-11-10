//*                 _              *
//*  _ __ ___  __ _(_) ___  _ __   *
//* | '__/ _ \/ _` | |/ _ \| '_ \  *
//* | | |  __/ (_| | | (_) | | | | *
//* |_|  \___|\__, |_|\___/|_| |_| *
//*           |___/                *
//===- include/pstore/region.hpp ------------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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
/// \file region.hpp

#ifndef PSTORE_REGION_HPP
#define PSTORE_REGION_HPP

#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

#include "pstore/memory_mapper.hpp"

namespace pstore {
    namespace region {

        inline std::uint64_t round_up (std::uint64_t x, std::uint64_t y) {
            return (x + y - 1) / y * y;
        }

        typedef std::shared_ptr<memory_mapper_base>  memory_mapper_ptr;


        //*                  _               _           _ _     _             *
        //*   _ __ ___  __ _(_) ___  _ __   | |__  _   _(_) | __| | ___ _ __   *
        //*  | '__/ _ \/ _` | |/ _ \| '_ \  | '_ \| | | | | |/ _` |/ _ \ '__|  *
        //*  | | |  __/ (_| | | (_) | | | | | |_) | |_| | | | (_| |  __/ |     *
        //*  |_|  \___|\__, |_|\___/|_| |_| |_.__/ \__,_|_|_|\__,_|\___|_|     *
        //*            |___/                                                   *

        /// A class which is responsible for creating the memory-mapped regions used by the
        /// data store. It is intended to decouple the creation of these object from the
        /// File and MemoryMapper classes.
        ///
        /// It tries to create regions which are as large as possible (in multiple of
        /// the "minimum" size, but no larger than "full" size to avoid requesting too much
        /// contiguous address space.

        template <typename File, typename MemoryMapper>
        struct region_builder {
        public:
            typedef std::vector<pstore::region::memory_mapper_ptr> container_type;

            /// \param file The file containing the data to be memory-mapped.
            /// \param full_size The number of bytes in a "full size" memory-mapped region.
            /// \param minimum_size The number of bytes in a "minimum size" memory-mapped
            /// region.
            region_builder (std::shared_ptr<File> file, std::uint64_t const full_size,
                            std::uint64_t const minimum_size) noexcept;

            // No assignment or copying.
            region_builder (region_builder const &) = delete;
            region_builder & operator=(region_builder const &) = delete;

            /// \param bytes_to_map The number of bytes to be mapped.
            /// \returns A collection of memory-mapped regions.
            auto operator() (std::uint64_t bytes_to_map) -> container_type;

            /// Creates one of more additional memory-mapped regions covering the file
            /// starting at the position given by 'offset' and extending for 'bytes_to_map'
            /// bytes.
            ///
            /// \param regions  An existing collection of memory-mapped regions that will be
            ///                 extended to contain the regions create to map the given file
            ///                 range.
            /// \param offset   The first byte of the file to be mapped by additional
            ///                 memory-mapped regions.
            /// \param bytes_to_map  The number of bytes in the file to be mapped by additional
            ///                      memory-mapped regions.
            void append (container_type * const regions, std::uint64_t offset,
                         std::uint64_t bytes_to_map);

        private:
            void push (container_type * const regions, std::uint64_t file_size, std::uint64_t offset,
                       std::uint64_t size);

            /// Checks the region-builder's post-condition that all of the regions are sorted
            /// and contiguous starting at an offset of 0.
            void check_regions_are_contiguous (container_type const & regions);

            /// The file for which a collection of memory-mapped regions is to be
            /// created.
            std::shared_ptr<File> file_;

            std::uint64_t const full_size_;    ///< The number of bytes in a "full size" memory-mapped region.
            std::uint64_t const minimum_size_; ///< The number of bytes in a "minimum size" memory-mapped region.
        };

        // region_builder
        // ~~~~~~~~~~~~~~
        template <typename File, typename MemoryMapper>
        region_builder<File, MemoryMapper>::region_builder (std::shared_ptr<File> file,
                                                            std::uint64_t const full_size,
                                                            std::uint64_t const minimum_size) noexcept
            : file_ (file),
              full_size_ (full_size),
              minimum_size_ (minimum_size) {

            assert (full_size >= minimum_size && full_size_ % minimum_size_ == 0);
        }

        // append
        // ~~~~~~
        template <typename File, typename MemoryMapper>
        void region_builder<File, MemoryMapper>::append (container_type * const regions,
                                                         std::uint64_t offset,
                                                         std::uint64_t bytes_to_map) {
            assert (regions != nullptr);
            assert (offset % minimum_size_ == 0);
            bytes_to_map = round_up (bytes_to_map, minimum_size_);
            assert (bytes_to_map % minimum_size_ == 0);

            // Zero or more regions whose size is a multiple of minimum-size but no
            // more than full-size.
            while (bytes_to_map > 0) {
                // Map no more than "full size" in one go.
                std::uint64_t const size = std::min (full_size_, bytes_to_map);

                bytes_to_map -= size;
                this->push (regions, bytes_to_map,
                            offset, // start of region
                            size);  // size of region
                offset += full_size_;
            }
            this->check_regions_are_contiguous (*regions);
        }

        // operator()
        // ~~~~~~~~~~
        template <typename File, typename MemoryMapper>
        auto region_builder<File, MemoryMapper>::operator() (std::uint64_t bytes_to_map)
            -> container_type {
            container_type regions;
            this->append (&regions, 0/*offset*/, bytes_to_map);
            return regions;
        }

        // push
        // ~~~~
        template <typename File, typename MemoryMapper>
        void region_builder<File, MemoryMapper>::push (container_type * const regions,
                                                       std::uint64_t /*file_size*/, std::uint64_t offset,
                                                       std::uint64_t size) {

            assert (regions != nullptr);
            assert (size >= minimum_size_);

            // (Note that we separately make pages read-only to guard against writing to committed
            // transactions: that's done by database::protect() rather than here.)
            bool const is_write_enabled = file_->is_writable ();
            auto mapper = std::make_shared<MemoryMapper> (*file_, is_write_enabled, offset, size);
            regions->push_back (mapper);
        }

        // check_regions_are_contiguous
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        template <typename File, typename MemoryMapper>
        void region_builder<File, MemoryMapper>::check_regions_are_contiguous (container_type const & regions) {
    #ifdef NDEBUG
            (void) regions;
    #else
            // Check that the regions are contiguous and sorted.
            std::uint64_t p = 0;
            for (pstore::region::memory_mapper_ptr region : regions) {
                assert (region->offset () == p);
                p += region->size ();
            }
    #endif
        }



        // small_files_enabled
        // ~~~~~~~~~~~~~~~~~~~
        bool small_files_enabled ();


        /*   __            _                    *
         *  / _| __ _  ___| |_ ___  _ __ _   _  *
         * | |_ / _` |/ __| __/ _ \| '__| | | | *
         * |  _| (_| | (__| || (_) | |  | |_| | *
         * |_|  \__,_|\___|\__\___/|_|   \__, | *
         *                               |___/  *
         */
        class factory {
        public:
            virtual ~factory () noexcept = default;

            /// \brief Creates the memory mapping objects for the given database file.
            ///
            /// Memory map the file regions. Creates an array of memory-mapped regions this will
            /// consist of a number whose size is equal to full_region_size followed by a number
            /// of min_size bytes each up to the size of the file. A file whose size is not an
            /// exact multiple of min_size will have a single mapped region which extends beyond
            /// the (original) end of the file.
            ///
            /// \returns A container of memory mapper objects.
            virtual std::vector<memory_mapper_ptr> init () = 0;

            virtual void add (std::vector<memory_mapper_ptr> * const regions,
                              std::uint64_t original_size,
                              std::uint64_t new_size) = 0;

            virtual std::shared_ptr <file::file_base> file () = 0;

            std::uint64_t full_size () const { return full_size_; }
            std::uint64_t min_size () const { return min_size_; }

        protected:
            /// \note full_size modulo minimum_size must be 0.
            /// \param full_size  The size of the largest memory-mapped file region.
            /// \param min_size  The size of the smallest memory-mapped file region.
            factory (std::uint64_t full_size, std::uint64_t min_size)
                    : full_size_ {full_size}
                    , min_size_ {min_size} {
                assert (full_size_ % min_size_ == 0);
            }

            template <typename File, typename MemoryMapper>
            auto create (std::shared_ptr <File> file) -> std::vector<memory_mapper_ptr>;

            template <typename File, typename MemoryMapper>
            void append (std::shared_ptr <File> file,
                         std::vector<memory_mapper_ptr> * const regions,
                         std::uint64_t original_size,
                         std::uint64_t new_size);

        private:
            std::uint64_t full_size_;
            std::uint64_t min_size_;
        };

        // create
        // ~~~~~~
        template <typename File, typename MemoryMapper>
        auto factory::create (std::shared_ptr<File> file) -> std::vector<memory_mapper_ptr> {

            // There's no lock on the file when we call the size() method here. However, the file
            // is only allowed to grow so if it changes then the worst outcome is that we end up
            // memory mapping more of it beyond the logical size.

            std::uint64_t const file_size = file->size ();
            region_builder<File, MemoryMapper> builder (file, this->full_size (),
                                                        this->min_size ());
            return builder (file_size);
        }

        // append
        // ~~~~~~
        template <typename File, typename MemoryMapper>
        void factory::append (std::shared_ptr<File> file,
                              std::vector<memory_mapper_ptr> * const regions,
                              std::uint64_t original_size, std::uint64_t new_size) {

            assert (new_size >= original_size);
            assert (regions != nullptr);

            auto const min_size = this->min_size ();
            region_builder<File, MemoryMapper> builder (file, this->full_size (), min_size);

            new_size = round_up (new_size, min_size);
            if (!small_files_enabled ()) {
                file->truncate (new_size);
            }
            builder.append (regions, original_size, round_up (new_size - original_size, min_size));
        }


        /*   __ _ _        _                        _    __            _                    *
         *  / _(_) | ___  | |__   __ _ ___  ___  __| |  / _| __ _  ___| |_ ___  _ __ _   _  *
         * | |_| | |/ _ \ | '_ \ / _` / __|/ _ \/ _` | | |_ / _` |/ __| __/ _ \| '__| | | | *
         * |  _| | |  __/ | |_) | (_| \__ \  __/ (_| | |  _| (_| | (__| || (_) | |  | |_| | *
         * |_| |_|_|\___| |_.__/ \__,_|___/\___|\__,_| |_|  \__,_|\___|\__\___/|_|   \__, | *
         *                                                                           |___/  *
         */
        class file_based_factory final : public factory {
        public:
            /// \param file An open file containing the data to be memory-mapped.
            /// \param full_size  The size of the largest memory-mapped file region.
            /// \param min_size  The size of the smallest memory-mapped file region.
            explicit file_based_factory (std::shared_ptr <file::file_handle> file,
                                         std::uint64_t full_size,
                                         std::uint64_t min_size);

            std::vector<memory_mapper_ptr> init () override;
            void add (std::vector<memory_mapper_ptr> * const regions,
                      std::uint64_t original_size,
                      std::uint64_t new_size) override;

            std::shared_ptr <file::file_base> file () override;

        private:
            std::shared_ptr <file::file_handle> file_;
        };


        /*                             _                        _    __            _                    *
         *  _ __ ___   ___ _ __ ___   | |__   __ _ ___  ___  __| |  / _| __ _  ___| |_ ___  _ __ _   _  *
         * | '_ ` _ \ / _ \ '_ ` _ \  | '_ \ / _` / __|/ _ \/ _` | | |_ / _` |/ __| __/ _ \| '__| | | | *
         * | | | | | |  __/ | | | | | | |_) | (_| \__ \  __/ (_| | |  _| (_| | (__| || (_) | |  | |_| | *
         * |_| |_| |_|\___|_| |_| |_| |_.__/ \__,_|___/\___|\__,_| |_|  \__,_|\___|\__\___/|_|   \__, | *
         *                                                                                       |___/  *
         */
        class mem_based_factory final : public factory {
        public:
            /// \param file An open file containing the data to be memory-mapped.
            /// \param full_size  The size of the largest memory-mapped file region.
            /// \param min_size  The size of the smallest memory-mapped file region.
            explicit mem_based_factory (std::shared_ptr <file::in_memory> file,
                                        std::uint64_t full_size,
                                        std::uint64_t min_size);

            std::vector<memory_mapper_ptr> init () override;
            void add (std::vector<memory_mapper_ptr> * const regions,
                      std::uint64_t original_size,
                      std::uint64_t new_size) override;

            std::shared_ptr <file::file_base> file () override;

        private:
            std::shared_ptr <file::in_memory> file_;
        };




        std::unique_ptr <factory> get_factory (std::shared_ptr <file::file_handle> file,
                                               std::uint64_t full_size,
                                               std::uint64_t min_size);

        std::unique_ptr <factory> get_factory (std::shared_ptr <file::in_memory> file,
                                               std::uint64_t full_size,
                                               std::uint64_t min_size);
    } // namespace region
} // namespace pstore
#endif // PSTORE_REGION_HPP
// eof: include/pstore/region.hpp
