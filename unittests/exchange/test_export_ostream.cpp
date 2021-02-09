//*                             _               _                             *
//*   _____  ___ __   ___  _ __| |_    ___  ___| |_ _ __ ___  __ _ _ __ ___   *
//*  / _ \ \/ / '_ \ / _ \| '__| __|  / _ \/ __| __| '__/ _ \/ _` | '_ ` _ \  *
//* |  __/>  <| |_) | (_) | |  | |_  | (_) \__ \ |_| | |  __/ (_| | | | | | | *
//*  \___/_/\_\ .__/ \___/|_|   \__|  \___/|___/\__|_|  \___|\__,_|_| |_| |_| *
//*           |_|                                                             *
//===- unittests/exchange/test_export_ostream.cpp -------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/export_ostream.hpp"

#include <sstream>

#include <gmock/gmock.h>

using namespace std::string_literals;

namespace {

    template <typename T>
    class UnsignedToString : public testing::Test {};

} // end anonymous namespace


#ifdef PSTORE_IS_INSIDE_LLVM
TYPED_TEST_CASE_P (UnsignedToString);
#else
TYPED_TEST_SUITE_P (UnsignedToString);
#endif // PSTORE_IS_INSIDE_LLVM

TYPED_TEST_P (UnsignedToString, Zero) {
    using namespace pstore::exchange::export_ns::details;
    base10storage<TypeParam> out;
    std::pair<char const *, char const *> const x = to_characters (TypeParam{0}, &out);
    EXPECT_EQ ((std::string{x.first, x.second}), "0");
}

TYPED_TEST_P (UnsignedToString, One) {
    using namespace pstore::exchange::export_ns::details;
    base10storage<TypeParam> out;
    auto const one = TypeParam{1};
    std::pair<char const *, char const *> const x = to_characters (one, &out);
    EXPECT_EQ ((std::string{x.first, x.second}), "1");
}

TYPED_TEST_P (UnsignedToString, Ten) {
    using namespace pstore::exchange::export_ns::details;
    base10storage<TypeParam> out;
    std::pair<char const *, char const *> const x = to_characters (TypeParam{10}, &out);
    EXPECT_EQ ((std::string{x.first, x.second}), "10");
}

TYPED_TEST_P (UnsignedToString, Max) {
    using namespace pstore::exchange::export_ns::details;
    base10storage<TypeParam> out;
    auto const max = std::numeric_limits<TypeParam>::max ();
    std::pair<char const *, char const *> const x = to_characters (max, &out);
    EXPECT_EQ ((std::string{x.first, x.second}), std::to_string (max));
}

using UnsignedTypes =
    testing::Types<unsigned char, unsigned int, unsigned long, unsigned long long>;

#ifdef PSTORE_IS_INSIDE_LLVM
REGISTER_TYPED_TEST_CASE_P (UnsignedToString, Zero, One, Ten, Max);
INSTANTIATE_TYPED_TEST_CASE_P (U2S, UnsignedToString, UnsignedTypes);
#else
REGISTER_TYPED_TEST_SUITE_P (UnsignedToString, Zero, One, Ten, Max);
INSTANTIATE_TYPED_TEST_SUITE_P (U2S, UnsignedToString, UnsignedTypes, );
#endif // PSTORE_IS_INSIDE_LLVM


namespace {

    class test_ostream : public pstore::exchange::export_ns::ostream_base {
    public:
        static constexpr auto buffer_size = std::size_t{3};

        test_ostream ()
                : ostream_base{buffer_size} {}
        test_ostream (test_ostream const &) = delete;
        test_ostream (test_ostream &&) = delete;

        ~test_ostream () noexcept override = default;

        test_ostream & operator= (test_ostream const &) = delete;
        test_ostream & operator= (test_ostream &&) = delete;

        MOCK_METHOD2 (flush_buffer, void (std::vector<char> const & buffer, std::size_t size));
    };

    class first_elements_are {
    public:
        explicit first_elements_are (std::string const & expected)
                : expected_{expected} {}
        bool operator() (std::vector<char> const & v) const {
            if (v.size () < expected_.size ()) {
                return false;
            }
            return std::equal (std::begin (expected_), std::end (expected_), std::begin (v));
        }

    private:
        std::string expected_;
    };

} // end anonymous namespace

TEST (OStream, WriteUnsigned) {
    test_ostream str;

    testing::InSequence seq;
    EXPECT_CALL (
        str, flush_buffer (testing::Truly (first_elements_are{"123"s}), test_ostream::buffer_size));
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"4"s}), std::size_t{1}));

    str << 1234U;
    str.flush ();
}

TEST (OStream, CZString) {
    test_ostream str;

    testing::InSequence seq;
    EXPECT_CALL (
        str, flush_buffer (testing::Truly (first_elements_are{"abc"s}), test_ostream::buffer_size));
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"de"s}), std::size_t{2}));

    str << "abcde";
    str.flush ();
}

TEST (OStream, String) {
    test_ostream str;

    testing::InSequence seq;
    EXPECT_CALL (
        str, flush_buffer (testing::Truly (first_elements_are{"abc"s}), test_ostream::buffer_size));
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"de"s}), std::size_t{2}));

    str << "abcde"s;
    str.flush ();
}

TEST (OStream, Span) {
    test_ostream str;

    testing::InSequence seq;
    EXPECT_CALL (
        str, flush_buffer (testing::Truly (first_elements_are{"abc"s}), test_ostream::buffer_size));
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"d"s}), std::size_t{1}));

    std::array<char const, 4U> const v{{'a', 'b', 'c', 'd'}};
    str.write (pstore::gsl::make_span (v));
    str.flush ();
}

TEST (OStream, LargeSpan) {
    std::array<char const, 7U> const v{{'a', 'b', 'c', 'd', 'e', 'f', 'g'}};

    test_ostream str;

    // TODO: here the code could optimize the case where more than a buffer's worth of data is
    // being written.
    testing::InSequence seq;
    EXPECT_CALL (
        str, flush_buffer (testing::Truly (first_elements_are{"abc"s}), test_ostream::buffer_size));
    EXPECT_CALL (
        str, flush_buffer (testing::Truly (first_elements_are{"def"s}), test_ostream::buffer_size));
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"g"s}), std::size_t{1}));

    str.write (pstore::gsl::make_span (v));
    str.flush ();
}

namespace {

    class ostringstream final : public pstore::exchange::export_ns::ostream_base {
    public:
        ostringstream () = default;
        ostringstream (ostringstream const &) = delete;
        ostringstream (ostringstream &&) = delete;

        ~ostringstream () noexcept override = default;

        ostringstream & operator= (ostringstream const &) = delete;
        ostringstream & operator= (ostringstream &&) = delete;

        std::string const & str ();

    private:
        std::string str_;
        void flush_buffer (std::vector<char> const & buffer, std::size_t size) override;
    };

    std::string const & ostringstream::str () {
        this->flush ();
        return str_;
    }

    void ostringstream::flush_buffer (std::vector<char> const & buffer, std::size_t size) {
        assert (size < std::numeric_limits<std::string::size_type>::max ());
        str_.append (buffer.data (), size);
    }

} // end anonymous namespace

namespace {

    // signed test values
    // ~~~~~~~~~~~~~~~~~~
    // Returns the values used to test signed integer types.
    template <typename T, typename = typename std::enable_if_t<std::is_signed<T>::value>>
    decltype (auto) signed_test_values () {
        return testing::Values (std::numeric_limits<T>::min (),
                                static_cast<T> (std::numeric_limits<T>::min () + 1), T{-1}, T{0},
                                T{1}, static_cast<T> (std::numeric_limits<T>::max () - 1),
                                std::numeric_limits<T>::max ());
    }

    // unsigned test values
    // ~~~~~~~~~~~~~~~~~~~~
    // Generates the values used to test unsigned integer types.
    template <typename T, typename = typename std::enable_if_t<std::is_unsigned<T>::value>>
    decltype (auto) unsigned_test_values () {
        return testing::Values (T{0}, T{1}, static_cast<T> (std::numeric_limits<T>::max () - 1U),
                                std::numeric_limits<T>::max ());
    }

    // check
    // ~~~~~
    // This function is the heart of the integer write tests. It works by comparing the output of
    // a stream type derived from pstore::exchange::export_ns::ostream_base with the output from
    // std::ostringstream and expecting that both produce the same result.
    template <typename T>
    void check (T const t) {
        ostringstream os;
        os << t;
        std::ostringstream stdos;
        stdos << t;
        EXPECT_EQ (os.str (), stdos.str ());
    }

} // end anonymous namespace

//*  _   _         _                  _   ___ _            _    *
//* | | | |_ _  __(_)__ _ _ _  ___ __| | / __| |_  ___ _ _| |_  *
//* | |_| | ' \(_-< / _` | ' \/ -_) _` | \__ \ ' \/ _ \ '_|  _| *
//*  \___/|_||_/__/_\__, |_||_\___\__,_| |___/_||_\___/_|  \__| *
//*                 |___/                                       *
class UnsignedShort : public testing::TestWithParam<unsigned short> {};

TEST_P (UnsignedShort, UnsignedTest) {
    SCOPED_TRACE ("UnsignedShort");
    check (GetParam ());
}

#ifdef PSTORE_IS_INSIDE_LLVM
INSTANTIATE_TEST_CASE_P (UnsignedShort, UnsignedShort, unsigned_test_values<unsigned short> (), );
#else
INSTANTIATE_TEST_SUITE_P (UnsignedShort, UnsignedShort, unsigned_test_values<unsigned short> ());
#endif

//*  ___ _                  _   ___ _            _    *
//* / __(_)__ _ _ _  ___ __| | / __| |_  ___ _ _| |_  *
//* \__ \ / _` | ' \/ -_) _` | \__ \ ' \/ _ \ '_|  _| *
//* |___/_\__, |_||_\___\__,_| |___/_||_\___/_|  \__| *
//*       |___/                                       *
class SignedShort : public testing::TestWithParam<short> {};

TEST_P (SignedShort, Signed) {
    SCOPED_TRACE ("SignedShort");
    check (GetParam ());
}

#ifdef PSTORE_IS_INSIDE_LLVM
INSTANTIATE_TEST_CASE_P (SignedShort, SignedShort, signed_test_values<short> (), );
#else
INSTANTIATE_TEST_SUITE_P (SignedShort, SignedShort, signed_test_values<short> ());
#endif

//*  _   _         _                  _   ___     _    *
//* | | | |_ _  __(_)__ _ _ _  ___ __| | |_ _|_ _| |_  *
//* | |_| | ' \(_-< / _` | ' \/ -_) _` |  | || ' \  _| *
//*  \___/|_||_/__/_\__, |_||_\___\__,_| |___|_||_\__| *
//*                 |___/                              *
class UnsignedInt : public testing::TestWithParam<unsigned> {};

TEST_P (UnsignedInt, Unsigned) {
    SCOPED_TRACE ("UnsignedInt");
    check (GetParam ());
}

#ifdef PSTORE_IS_INSIDE_LLVM
INSTANTIATE_TEST_CASE_P (UnsignedInt, UnsignedInt, unsigned_test_values<unsigned> (), );
#else
INSTANTIATE_TEST_SUITE_P (UnsignedInt, UnsignedInt, unsigned_test_values<unsigned> ());
#endif

//*  ___ _                  _   ___     _    *
//* / __(_)__ _ _ _  ___ __| | |_ _|_ _| |_  *
//* \__ \ / _` | ' \/ -_) _` |  | || ' \  _| *
//* |___/_\__, |_||_\___\__,_| |___|_||_\__| *
//*       |___/                              *
class SignedInt : public testing::TestWithParam<int> {};

TEST_P (SignedInt, Signed) {
    SCOPED_TRACE ("SignedInt");
    check (GetParam ());
}

#ifdef PSTORE_IS_INSIDE_LLVM
INSTANTIATE_TEST_CASE_P (SignedInt, SignedInt, signed_test_values<int> (), );
#else
INSTANTIATE_TEST_SUITE_P (SignedInt, SignedInt, signed_test_values<int> ());
#endif

//*  _   _         _                  _   _                   *
//* | | | |_ _  __(_)__ _ _ _  ___ __| | | |   ___ _ _  __ _  *
//* | |_| | ' \(_-< / _` | ' \/ -_) _` | | |__/ _ \ ' \/ _` | *
//*  \___/|_||_/__/_\__, |_||_\___\__,_| |____\___/_||_\__, | *
//*                 |___/                              |___/  *
class UnsignedLong : public testing::TestWithParam<unsigned long> {};

TEST_P (UnsignedLong, Unsigned) {
    SCOPED_TRACE ("UnsignedLong");
    check (GetParam ());
}

#ifdef PSTORE_IS_INSIDE_LLVM
INSTANTIATE_TEST_CASE_P (UnsignedLong, UnsignedLong, unsigned_test_values<unsigned long> (), );
#else
INSTANTIATE_TEST_SUITE_P (UnsignedLong, UnsignedLong, unsigned_test_values<unsigned long> ());
#endif

//*  ___ _                  _   _                   *
//* / __(_)__ _ _ _  ___ __| | | |   ___ _ _  __ _  *
//* \__ \ / _` | ' \/ -_) _` | | |__/ _ \ ' \/ _` | *
//* |___/_\__, |_||_\___\__,_| |____\___/_||_\__, | *
//*       |___/                              |___/  *
class SignedLong : public testing::TestWithParam<long> {};

TEST_P (SignedLong, Signed) {
    SCOPED_TRACE ("SignedLong");
    check (GetParam ());
}

#ifdef PSTORE_IS_INSIDE_LLVM
INSTANTIATE_TEST_CASE_P (SignedLong, SignedLong, signed_test_values<long> (), );
#else
INSTANTIATE_TEST_SUITE_P (SignedLong, SignedLong, signed_test_values<long> ());
#endif

//*  _   _         _                  _   _                    _                   *
//* | | | |_ _  __(_)__ _ _ _  ___ __| | | |   ___ _ _  __ _  | |   ___ _ _  __ _  *
//* | |_| | ' \(_-< / _` | ' \/ -_) _` | | |__/ _ \ ' \/ _` | | |__/ _ \ ' \/ _` | *
//*  \___/|_||_/__/_\__, |_||_\___\__,_| |____\___/_||_\__, | |____\___/_||_\__, | *
//*                 |___/                              |___/                |___/  *
class UnsignedLongLong : public testing::TestWithParam<unsigned long long> {};

TEST_P (UnsignedLongLong, Unsigned) {
    SCOPED_TRACE ("UnsignedLongLong");
    check (GetParam ());
}

#ifdef PSTORE_IS_INSIDE_LLVM
INSTANTIATE_TEST_CASE_P (UnsignedLongLong, UnsignedLongLong,
                         unsigned_test_values<unsigned long long> (), );
#else
INSTANTIATE_TEST_SUITE_P (UnsignedLongLong, UnsignedLongLong,
                          unsigned_test_values<unsigned long long> ());
#endif

//*  ___ _                  _   _                    _                   *
//* / __(_)__ _ _ _  ___ __| | | |   ___ _ _  __ _  | |   ___ _ _  __ _  *
//* \__ \ / _` | ' \/ -_) _` | | |__/ _ \ ' \/ _` | | |__/ _ \ ' \/ _` | *
//* |___/_\__, |_||_\___\__,_| |____\___/_||_\__, | |____\___/_||_\__, | *
//*       |___/                              |___/                |___/  *
class SignedLongLong : public testing::TestWithParam<long long> {};

TEST_P (SignedLongLong, Signed) {
    SCOPED_TRACE ("SignedLong");
    check (GetParam ());
}

#ifdef PSTORE_IS_INSIDE_LLVM
INSTANTIATE_TEST_CASE_P (SignedLongLong, SignedLongLong, signed_test_values<long long> (), );
#else
INSTANTIATE_TEST_SUITE_P (SignedLongLong, SignedLongLong, signed_test_values<long long> ());
#endif
