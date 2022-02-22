//===- unittests/core/test_db_archive.cpp ---------------------------------===//
//*      _ _                      _     _            *
//*   __| | |__     __ _ _ __ ___| |__ (_)_   _____  *
//*  / _` | '_ \   / _` | '__/ __| '_ \| \ \ / / _ \ *
//* | (_| | |_) | | (_| | | | (__| | | | |\ V /  __/ *
//*  \__,_|_.__/   \__,_|_|  \___|_| |_|_| \_/ \___| *
//*                                                  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/core/db_archive.hpp"

// Stadard includes
#include <array>
#include <cstdint>

// 3rd party includes
#include <gmock/gmock.h>

// pstore public includes
#include "pstore/support/gsl.hpp"
#include "pstore/core/transaction.hpp"

// Local test includes
#include "empty_store.hpp"

namespace {

    pstore::typed_address<std::uint64_t> append_uint64 (pstore::transaction_base & transaction,
                                                        std::uint64_t v) {
        auto addr = pstore::typed_address<std::uint64_t>::null ();
        std::shared_ptr<std::uint64_t> ptr;
        std::tie (ptr, addr) = transaction.alloc_rw<std::uint64_t> ();
        *ptr = v;
        return addr;
    }

    class DbArchive : public testing::Test {
    protected:
        InMemoryStore store_;
    };

} // end anonymous namespace

TEST_F (DbArchive, ReadASingleUint64) {
    pstore::database db{store_.file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    std::uint64_t v1 = UINT64_C (0xF0F0F0F0F0F0F0F0);

    // Append v1 to the store (we don't need to have committed the transaction to be able to access
    // its contents).
    mock_mutex mutex;
    auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});
    pstore::typed_address<std::uint64_t> addr = append_uint64 (transaction, v1);

    // Now try reading it back again using a serializer.
    pstore::serialize::archive::database_reader archive (db, addr.to_address ());
    std::uint64_t const v2 = pstore::serialize::read<std::uint64_t> (archive);
    EXPECT_EQ (v1, v2);
}

TEST_F (DbArchive, ReadAUint64Span) {
    using ::testing::ContainerEq;

    pstore::database db{store_.file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    std::array<std::uint64_t, 2> const original{{
        UINT64_C (0xF0F0F0F0F0F0F0F0),
        UINT64_C (0xFEEDFACECAFEBEEF),
    }};

    // Append 'original' to the store.
    mock_mutex mutex;
    auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});
    pstore::typed_address<std::uint64_t> addr = append_uint64 (transaction, original[0]);
    append_uint64 (transaction, original[1]);

    // Now the read the array back again.
    pstore::serialize::archive::database_reader archive (db, addr.to_address ());
    std::array<std::uint64_t, 2> actual{{UINT64_C (0)}};
    pstore::serialize::read (archive, ::pstore::gsl::span<std::uint64_t>{actual});

    EXPECT_THAT (original, ContainerEq (actual));
}

TEST_F (DbArchive, WriteASingleUint64) {
    pstore::database db{store_.file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    std::uint64_t const original = UINT64_C (0xF0F0F0F0F0F0F0F0);

    // Write 'original' to the store using a serializer.
    mock_mutex mutex;
    auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});
    auto archive = pstore::serialize::archive::make_writer (transaction);
    auto where = pstore::typed_address<std::uint64_t>::make (db.size ());
    pstore::serialize::write (archive, original);

    // Now read that value back again using the raw pstore API and check that the round-trip was
    // successful.
    auto where2 = pstore::typed_address<std::uint64_t>::make (
        where.absolute () + pstore::calc_alignment (where.absolute (), alignof (std::uint64_t)));
    auto actual = db.getro (where2);
    EXPECT_EQ (original, *actual);
}

TEST_F (DbArchive, WriteAUint64Span) {
    pstore::database db{store_.file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    std::array<std::uint64_t, 2> const original{{
        UINT64_C (0xF0F0F0F0F0F0F0F0),
        UINT64_C (0xFEEDFACECAFEBEEF),
    }};

    // Write the 'original' array span to the store using a serializer.
    mock_mutex mutex;
    auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});
    auto where = pstore::address{db.size ()};
    pstore::serialize::write (pstore::serialize::archive::make_writer (transaction),
                              pstore::gsl::make_span (original));

    // Now read that value back again using the raw pstore API and check that the round-trip was
    // successful.
    auto where2 = pstore::typed_address<std::uint64_t>::make (
        where.absolute () + pstore::calc_alignment (where.absolute (), alignof (std::uint64_t)));
    std::shared_ptr<std::uint64_t const> actual = db.getro (where2, 2);
    EXPECT_EQ (original[0], (actual.get ())[0]);
    EXPECT_EQ (original[1], (actual.get ())[1]);
}


namespace {

    class DbArchiveWriteSpan : public testing::Test {
    protected:
        class mock_transaction : public pstore::transaction<std::unique_lock<mock_mutex>> {
        public:
            using inherited = pstore::transaction<std::unique_lock<mock_mutex>>;
            mock_transaction (pstore::database & db, inherited::lock_type && lock)
                    : inherited (db, std::move (lock)) {}

            MOCK_METHOD2 (allocate, pstore::address (std::uint64_t /*size*/, unsigned /*align*/));
            pstore::address base_allocate (std::uint64_t size, unsigned align) {
                return inherited::allocate (size, align);
            }
        };

        InMemoryStore store_;
    };

} // end anonymous namespace

TEST_F (DbArchiveWriteSpan, WriteUint64Span) {
    using ::testing::_;
    using ::testing::Invoke;

    pstore::database db{store_.file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    std::array<std::uint64_t, 2> original{{
        UINT64_C (0x0011223344556677),
        UINT64_C (0x8899AABBCCDDEEFF),
    }};
    auto const span = ::pstore::gsl::make_span (original);

    mock_mutex mutex;
    mock_transaction transaction (db, std::unique_lock<mock_mutex>{mutex});
    // All calls to transaction.alloc_rw are forwarded to the real implementation. We
    // expect there to be a single call to alloc_rw() which is suitable for the space
    // and alignment requirement of the entire span.
    auto invoke_base_allocate = Invoke (&transaction, &mock_transaction::base_allocate);
    EXPECT_CALL (transaction, allocate (_, _)).WillRepeatedly (invoke_base_allocate);
    EXPECT_CALL (transaction, allocate (sizeof (original), alignof (std::uint64_t)))
        .Times (1)
        .WillOnce (invoke_base_allocate);

    // Write the span.
    using namespace pstore::serialize;
    write (archive::make_writer (transaction), span);
}

namespace {


    class DbArchiveReadSpan : public testing::Test {
    private:
        using unique_pointer = pstore::unique_pointer<void const>;

    protected:
        class mock_database : public pstore::database {
        public:
            explicit mock_database (std::shared_ptr<pstore::file::in_memory> const & file)
                    : pstore::database{file} {
                this->set_vacuum_mode (pstore::database::vacuum_mode::disabled);
            }

            MOCK_CONST_METHOD4 (get, std::shared_ptr<void const> (pstore::address, std::size_t,
                                                                  bool /*is_initialized*/,
                                                                  bool /*is_writable*/));
            MOCK_CONST_METHOD3 (getu, unique_pointer (pstore::address, std::size_t,
                                                      bool /*is_initialized*/));

            auto base_get (pstore::address const & addr, std::size_t size, bool is_initialized,
                           bool is_writable) const -> std::shared_ptr<void const> {
                return pstore::database::get (addr, size, is_initialized, is_writable);
            }
            auto base_getu (pstore::address const & addr, std::size_t size,
                            bool is_initialized) const -> unique_pointer {
                return pstore::database::getu (addr, size, is_initialized);
            }
        };

        InMemoryStore store_;
    };

} // end anonymous namespace

// A workaround for an issue with some versions GNU libc++ that was exposed by the GCC 5.5.0 CI
// build. Prior to a change in 2018-09-18, std::is_default_constructible<> would return true for
// pstore::unique_pointer<> but a static assertion would fire when the default constructor was used.
// This code injects a specialization of BuiltinDefaultValue<> into Google Mock which bypasses
// std::is_default_constructible<>.
#if defined(__GLIBCXX__) && __GLIBCXX__ < 20180918
namespace testing {
    namespace internal {

        template <typename T, typename D>
        class BuiltInDefaultValue<std::unique_ptr<T, D>> {
        public:
            static constexpr bool Exists () noexcept { return is_default_constructible; }
            static std::unique_ptr<T, D> Get () {
                return BuiltInDefaultValueGetter<std::unique_ptr<T, D>,
                                                 is_default_constructible>::Get ();
            }

        private:
            static constexpr bool is_default_constructible = false;
        };

    } // end namespace internal
} // end namespace testing
#endif // defined (__GLIBCXX__) && __GLIBCXX__ < 20180918

TEST_F (DbArchiveReadSpan, ReadUint64Span) {
    using testing::_;
    using testing::Invoke;

    testing::NiceMock<mock_database> db{store_.file ()};

    // Calls to db.get() are forwarded to the real implementation.
    ON_CALL (db, get (_, _, _, _)).WillByDefault (Invoke (&db, &mock_database::base_get));

    // All calls to db.getu() are forwarded to the real implementation.
    auto invoke_base_getu = Invoke (&db, &mock_database::base_getu);
    ON_CALL (db, getu (_, _, _)).WillByDefault (invoke_base_getu);

    // Append 'original' to the store.
    std::array<std::uint64_t, 2> const original{{
        UINT64_C (0xF0F0F0F0F0F0F0F0),
        UINT64_C (0xFEEDFACECAFEBEEF),
    }};

    mock_mutex mutex;
    auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});
    pstore::typed_address<std::uint64_t> addr = append_uint64 (transaction, original[0]);
    append_uint64 (transaction, original[1]);

    // Now use the serializer to read a span of two uint64_ts. We expect the database.getu()
    // method to be called exactly once for this operation.
    std::array<std::uint64_t, 2> actual;
    EXPECT_CALL (db, getu (addr.to_address (), sizeof (actual), true /*initialized*/))
        .Times (1)
        .WillOnce (invoke_base_getu);

    using namespace pstore::serialize;
    read (archive::database_reader (db, addr.to_address ()),
          pstore::gsl::span<std::uint64_t>{actual});

    EXPECT_THAT (actual, testing::ContainerEq (original));
}
