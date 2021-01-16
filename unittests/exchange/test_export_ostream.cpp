#include "pstore/exchange/export_ostream.hpp"

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
#endif

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

#if PSTORE_IS_INSIDE_LLVM
REGISTER_TYPED_TEST_CASE_P (UnsignedToString, Zero, One, Ten, Max);
INSTANTIATE_TYPED_TEST_CASE_P (U2S, UnsignedToString, UnsignedTypes);
#else
REGISTER_TYPED_TEST_SUITE_P (UnsignedToString, Zero, One, Ten, Max);
INSTANTIATE_TYPED_TEST_SUITE_P (U2S, UnsignedToString, UnsignedTypes, );
#endif


namespace {

    class string_ostream : public pstore::exchange::export_ns::ostream_base {
    public:
        static constexpr auto buffer_size = std::size_t{3};

        string_ostream ()
                : ostream_base{buffer_size} {}
        string_ostream (string_ostream const &) = delete;
        string_ostream (string_ostream &&) = delete;

        ~string_ostream () noexcept override = default;

        string_ostream & operator= (string_ostream const &) = delete;
        string_ostream & operator= (string_ostream &&) = delete;

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
    string_ostream str;

    testing::InSequence seq;
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"123"s}),
                                    string_ostream::buffer_size));
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"4"s}), std::size_t{1}));

    str << 1234U;
    str.flush ();
}

TEST (OStream, CZString) {
    string_ostream str;

    testing::InSequence seq;
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"abc"s}),
                                    string_ostream::buffer_size));
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"de"s}), std::size_t{2}));

    str << "abcde";
    str.flush ();
}

TEST (OStream, String) {
    string_ostream str;

    testing::InSequence seq;
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"abc"s}),
                                    string_ostream::buffer_size));
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"de"s}), std::size_t{2}));

    str << "abcde"s;
    str.flush ();
}

TEST (OStream, Span) {
    string_ostream str;

    testing::InSequence seq;
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"abc"s}),
                                    string_ostream::buffer_size));
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"d"s}), std::size_t{1}));

    std::array<char const, 4U> const v{{'a', 'b', 'c', 'd'}};
    str.write (pstore::gsl::make_span (v));
    str.flush ();
}

TEST (OStream, LargeSpan) {
    std::array<char const, 7U> const v{{'a', 'b', 'c', 'd', 'e', 'f', 'g'}};

    string_ostream str;

    // TODO: here the code could optimize the case where more than a buffer's worth of data is
    // being written.
    testing::InSequence seq;
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"abc"s}),
                                    string_ostream::buffer_size));
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"def"s}),
                                    string_ostream::buffer_size));
    EXPECT_CALL (str, flush_buffer (testing::Truly (first_elements_are{"g"s}), std::size_t{1}));

    str.write (pstore::gsl::make_span (v));
    str.flush ();
}
