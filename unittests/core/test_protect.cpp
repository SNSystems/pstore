//===- unittests/core/test_protect.cpp ------------------------------------===//
//*                  _            _    *
//*  _ __  _ __ ___ | |_ ___  ___| |_  *
//* | '_ \| '__/ _ \| __/ _ \/ __| __| *
//* | |_) | | | (_) | ||  __/ (__| |_  *
//* | .__/|_|  \___/ \__\___|\___|\__| *
//* |_|                                *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/core/transaction.hpp"

// 3rd party includes
#include <gmock/gmock.h>

// pstore includes
#include "pstore/os/memory_mapper.hpp"

// Local private includes
#include "empty_store.hpp"

namespace {

    class fixed_page_size final : public pstore::system_page_size_interface {
    public:
        MOCK_CONST_METHOD0 (get, unsigned ());
    };


    class mock_mapper : public pstore::in_memory_mapper {
    public:
        mock_mapper (pstore::file::in_memory & file, bool write_enabled, std::uint64_t offset,
                     std::uint64_t length)
                : pstore::in_memory_mapper (file, write_enabled, offset, length) {}

        MOCK_METHOD2 (read_only, void (void * addr, std::size_t len));
    };


    class mock_region_factory final : public pstore::region::factory {
    public:
        mock_region_factory (std::shared_ptr<pstore::file::in_memory> file, std::uint64_t full_size,
                             std::uint64_t min_size)
                : pstore::region::factory (full_size, min_size)
                , file_ (std::move (file)) {

            PSTORE_ASSERT (full_size >= min_size);
            PSTORE_ASSERT (full_size % pstore::address::segment_size == 0);
            PSTORE_ASSERT (min_size % pstore::address::segment_size == 0);
        }

        auto init () -> std::vector<pstore::region::memory_mapper_ptr> override {
            return this->create<pstore::file::in_memory, mock_mapper> (file_);
        }

        void add (pstore::gsl::not_null<std::vector<pstore::region::memory_mapper_ptr> *> regions,
                  std::uint64_t original_size, std::uint64_t new_size) override {
            this->append<pstore::file::in_memory, mock_mapper> (file_, regions, original_size,
                                                                new_size);
        }

        std::shared_ptr<pstore::file::file_base> file () override { return file_; }

    private:
        std::shared_ptr<pstore::file::in_memory> file_;
    };

} // end anonymous namespace


namespace {

    template <typename T, typename U>
    std::shared_ptr<T> cast (std::shared_ptr<U> p) {
#ifdef PSTORE_CPP_RTTI
        return std::dynamic_pointer_cast<T> (p);
#else
        return std::static_pointer_cast<T> (p);
#endif
    }

    class EmptyStore : public ::testing::Test {
    protected:
        in_memory_store store_;
    };

} // end anonymous namespace


TEST_F (EmptyStore, ProtectAllOfOneRegion) {
    using ::testing::_;
    using ::testing::Ref;
    using ::testing::Return;

    auto const fixed_page_size_bytes = 4096U;
    auto page_size = std::make_unique<fixed_page_size> ();
    EXPECT_CALL (*page_size, get ()).WillRepeatedly (Return (fixed_page_size_bytes));

    // Create the data store instance. It will use 4K pages mapped using mock_mapper instances.
    pstore::database db{store_.file (), std::move (page_size),
                        std::make_unique<mock_region_factory> (store_.file (),
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

    // If "POSIX small file" mode is enabled, the file will be smaller than a VM page (4K), so
    // read_only() won't be called at all.
    if (pstore::database::small_files_enabled ()) {
        EXPECT_CALL (*r0.get (), read_only (_, _)).Times (0);
    } else {
        void * addr = reinterpret_cast<std::uint8_t *> (store_.file ()->data ().get ()) +
                      fixed_page_size_bytes;
        EXPECT_CALL (*r0.get (), read_only (addr, store_.file ()->size () - fixed_page_size_bytes))
            .Times (1);
    }

    db.protect (pstore::address::null (), pstore::address{store_.file ()->size ()});
}

TEST_F (EmptyStore, ProtectAllOfTwoRegions) {
    using ::testing::Ref;
    using ::testing::Return;

    auto const fixed_page_size_bytes = 4096U;
    auto page_size = std::make_unique<fixed_page_size> ();
    EXPECT_CALL (*page_size, get ()).WillRepeatedly (Return (fixed_page_size_bytes));

    // Create the data store instance. It will use 4K pages mapped using mock_mapper instances.
    pstore::database db{store_.file (), std::move (page_size),
                        std::make_unique<mock_region_factory> (store_.file (),
                                                               pstore::address::segment_size,
                                                               pstore::address::segment_size)};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);


    mock_mutex mutex;
    auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});
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
                 read_only (reinterpret_cast<std::uint8_t *> (store_.file ()->data ().get ()) +
                                fixed_page_size_bytes,
                            r0_protect_size))
        .Times (1);

    EXPECT_CALL (*r1.get (),
                 read_only (reinterpret_cast<std::uint8_t *> (store_.file ()->data ().get ()) +
                                pstore::address::segment_size,
                            4096U))
        .Times (1);

    transaction.commit ();
}
