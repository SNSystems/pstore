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
        ostringstream ()
                : ostream_base{} {}
        ostringstream (ostringstream const &) = delete;
        ostringstream (ostringstream &&) = delete;

        ~ostringstream () noexcept override = default;

        ostringstream & operator= (ostringstream const &) = delete;
        ostringstream & operator= (ostringstream &&) = delete;

        std::string const & str () {
            this->flush ();
            return str_;
        }

    private:
        std::string str_;

        void flush_buffer (std::vector<char> const & buffer, std::size_t size) override {
            assert (size < std::numeric_limits<std::string::size_type>::max ());
            str_.append (buffer.data (), size);
        }
    };

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


class UnsignedShort : public testing::TestWithParam<unsigned short> {};
TEST_P (UnsignedShort, UnsignedTest) {
    SCOPED_TRACE ("UnsignedShort");
    check (GetParam ());
}
INSTANTIATE_TEST_SUITE_P (UnsignedShort, UnsignedShort,
                          testing::Values (static_cast<unsigned short> (0),
                                           static_cast<unsigned short> (1),
                                           std::numeric_limits<unsigned short>::max ()));


class SignedShort : public testing::TestWithParam<short> {};
TEST_P (SignedShort, Signed) {
    SCOPED_TRACE ("SignedShort");
    check (GetParam ());
}
INSTANTIATE_TEST_SUITE_P (SignedShort, SignedShort,
                          testing::Values (std::numeric_limits<short>::min (),
                                           static_cast<short> (-1), static_cast<short> (0),
                                           static_cast<short> (1),
                                           std::numeric_limits<short>::max ()));



class UnsignedInt : public testing::TestWithParam<unsigned> {};
TEST_P (UnsignedInt, Unsigned) {
    SCOPED_TRACE ("UnsignedInt");
    check (GetParam ());
}
INSTANTIATE_TEST_SUITE_P (UnsignedInt, UnsignedInt,
                          testing::Values (0U, 1U, std::numeric_limits<unsigned>::max ()));

class SignedInt : public testing::TestWithParam<int> {};
TEST_P (SignedInt, Signed) {
    SCOPED_TRACE ("SignedInt");
    check (GetParam ());
}
INSTANTIATE_TEST_SUITE_P (SignedInt, SignedInt,
                          testing::Values (std::numeric_limits<int>::min (), -1, 0, 1,
                                           std::numeric_limits<int>::max ()));



class UnsignedLong : public testing::TestWithParam<unsigned long> {};
TEST_P (UnsignedLong, Unsigned) {
    SCOPED_TRACE ("UnsignedLong");
    check (GetParam ());
}
INSTANTIATE_TEST_SUITE_P (UnsignedLong, UnsignedLong,
                          testing::Values (0UL, 1UL, std::numeric_limits<unsigned long>::max ()));

class SignedLong : public testing::TestWithParam<long> {};
TEST_P (SignedLong, Signed) {
    SCOPED_TRACE ("SignedLong");
    check (GetParam ());
}
INSTANTIATE_TEST_SUITE_P (SignedLong, SignedLong,
                          testing::Values (std::numeric_limits<long>::min (), -1L, 0L, 1L,
                                           std::numeric_limits<long>::max ()));

class UnsignedLongLong : public testing::TestWithParam<unsigned long long> {};
TEST_P (UnsignedLongLong, Unsigned) {
    SCOPED_TRACE ("UnsignedLongLong");
    check (GetParam ());
}
INSTANTIATE_TEST_SUITE_P (UnsignedLongLong, UnsignedLongLong,
                          testing::Values (0ULL, 1ULL,
                                           std::numeric_limits<unsigned long long>::max ()));

class SignedLongLong : public testing::TestWithParam<long long> {};
TEST_P (SignedLongLong, Signed) {
    SCOPED_TRACE ("SignedLong");
    check (GetParam ());
}
INSTANTIATE_TEST_SUITE_P (SignedLongLong, SignedLongLong,
                          testing::Values (std::numeric_limits<long long>::min (), -1LL, 0LL, 1LL,
                                           std::numeric_limits<long long>::max ()));
