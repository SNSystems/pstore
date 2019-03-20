//*                  _            _    *
//*  _ __  _ __ ___ | |_ ___  ___| |_  *
//* | '_ \| '__/ _ \| __/ _ \/ __| __| *
//* | |_) | | | (_) | ||  __/ (__| |_  *
//* | .__/|_|  \___/ \__\___|\___|\__| *
//* |_|                                *
//===- unittests/core/test_protect.cpp ------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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

#include "pstore/core/transaction.hpp"

#include "gmock/gmock.h"

#include "pstore/core/memory_mapper.hpp"
#include "empty_store.hpp"
#include "mock_mutex.hpp"

namespace {
    class fixed_page_size final : public pstore::system_page_size_interface {
    public:
        MOCK_CONST_METHOD0 (get, unsigned());
    };


    class mock_mapper : public pstore::in_memory_mapper {
    public:
        mock_mapper (pstore::file::in_memory & file, bool write_enabled, std::uint64_t offset,
                     std::uint64_t length)
                : pstore::in_memory_mapper (file, write_enabled, offset, length) {}

        MOCK_METHOD2 (read_only, void(void * addr, std::size_t len));
    };


    class mock_region_factory final : public pstore::region::factory {
    public:
        mock_region_factory (std::shared_ptr<pstore::file::in_memory> file, std::uint64_t full_size,
                             std::uint64_t min_size)
                : pstore::region::factory (full_size, min_size)
                , file_ (std::move (file)) {

            assert (full_size >= min_size);
            assert (full_size % pstore::address::segment_size == 0);
            assert (min_size % pstore::address::segment_size == 0);
        }

        auto init () -> std::vector<pstore::region::memory_mapper_ptr> override {
            return this->create<pstore::file::in_memory, mock_mapper> (file_);
        }

        void add (std::vector<pstore::region::memory_mapper_ptr> * const regions,
                  std::uint64_t original_size, std::uint64_t new_size) override {

            this->append<pstore::file::in_memory, mock_mapper> (file_, regions, original_size,
                                                                new_size);
        }

        std::shared_ptr<pstore::file::file_base> file () override { return file_; }

    private:
        std::shared_ptr<pstore::file::in_memory> file_;
    };
} // namespace


namespace {
    template <typename T, typename U>
    std::shared_ptr<T> cast (std::shared_ptr<U> p) {
#ifdef PSTORE_CPP_RTTI
        return std::dynamic_pointer_cast<T> (p);
#else
        return std::static_pointer_cast<T> (p);
#endif
    }
} // end anonymous namespace


TEST_F (EmptyStore, ProtectAllOfOneRegion) {
    using ::testing::_;
    using ::testing::Ref;
    using ::testing::Return;

    auto const fixed_page_size_bytes = 4096U;
    auto page_size = std::make_unique<fixed_page_size> ();
    EXPECT_CALL (*page_size, get ()).WillRepeatedly (Return (fixed_page_size_bytes));

    // Create the data store instance. It will use 4K pages mapped using mock_mapper instances.
    pstore::database db{this->file (), std::move (page_size),
                        std::make_unique<mock_region_factory> (this->file (),
                                                               pstore::storage::min_region_size,
                                                               pstore::storage::min_region_size)};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);


    pstore::storage::region_container const & regions = db.storage ().regions ();
    EXPECT_EQ (1U, regions.size ()) << "Expected the store to use 1 region";

    // I expect db.protect() to call mock_mapper::read_only() once with
    // - The first parameter should equal one page into the memory block (the first page of the
    //  data store must remain writable.
    // - The second should be the size of the file minus the size of that missing first page.
    auto r0 = cast<mock_mapper> (regions.at (0));
    void * addr =
        reinterpret_cast<std::uint8_t *> (this->file ()->data ().get ()) + fixed_page_size_bytes;

    // If "POSIX small file" mode is enabled, the file will be smaller than a VM page (4K), so
    // read_only() won't be called at all.
    if (pstore::database::small_files_enabled ()) {
        EXPECT_CALL (*r0.get (), read_only (_, _)).Times (0);
    } else {
        EXPECT_CALL (*r0.get (), read_only (addr, this->file ()->size () - fixed_page_size_bytes))
            .Times (1);
    }

    db.protect (pstore::address::null (), pstore::address::make (this->file ()->size ()));
}

TEST_F (EmptyStore, ProtectAllOfTwoRegions) {
    using ::testing::Ref;
    using ::testing::Return;

    auto const fixed_page_size_bytes = 4096U;
    auto page_size = std::make_unique<fixed_page_size> ();
    EXPECT_CALL (*page_size, get ()).WillRepeatedly (Return (fixed_page_size_bytes));

    // Create the data store instance. It will use 4K pages mapped using mock_mapper instances.
    pstore::database db{this->file (), std::move (page_size),
                        std::make_unique<mock_region_factory> (this->file (),
                                                               pstore::address::segment_size,
                                                               pstore::address::segment_size)};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);


    mock_mutex mutex;
    auto transaction = pstore::begin (db, std::unique_lock<mock_mutex>{mutex});
    transaction.allocate (pstore::address::segment_size + 4096U /*size*/, 1 /*align*/);

    pstore::storage::region_container const & regions = db.storage ().regions ();
    EXPECT_EQ (2U, regions.size ()) << "Expected the store to use two regions";
    auto r0 = cast<mock_mapper> (regions.at (0));
    auto r1 = cast<mock_mapper> (regions.at (1));

    // I expect transaction commit to call read_only() on the first memory-mapped region once with:
    // - The first parameter equal to one page into the memory block (the first page of the
    //  data store must remain writable.
    // - The second should be the size of the region minus the size of that missing first page.
    std::uint64_t r0_protect_size = r0->size () - fixed_page_size_bytes;
    EXPECT_CALL (*r0.get (),
                 read_only (reinterpret_cast<std::uint8_t *> (this->file ()->data ().get ()) +
                                fixed_page_size_bytes,
                            r0_protect_size))
        .Times (1);

    EXPECT_CALL (*r1.get (),
                 read_only (reinterpret_cast<std::uint8_t *> (this->file ()->data ().get ()) +
                                pstore::address::segment_size,
                            4096U))
        .Times (1);

    transaction.commit ();
}
