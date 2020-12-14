#include "pstore/exchange/export_ostream.hpp"

#include <gtest/gtest.h>

namespace {

    template <typename T>
    class UnsignedToString : public testing::Test {};

} // end anonymous namespace

TYPED_TEST_SUITE_P (UnsignedToString);

TYPED_TEST_P (UnsignedToString, Zero) {
    using namespace pstore::exchange::export_ns::details;
    base10storage<TypeParam> out;
    std::pair<char const *, char const *> const x = u2s (TypeParam{0}, &out);
    EXPECT_EQ ((std::string{x.first, x.second}), "0");
}

TYPED_TEST_P (UnsignedToString, One) {
    using namespace pstore::exchange::export_ns::details;
    base10storage<TypeParam> out;
    auto const one = TypeParam{1};
    std::pair<char const *, char const *> const x = u2s (one, &out);
    EXPECT_EQ ((std::string{x.first, x.second}), "1");
}

TYPED_TEST_P (UnsignedToString, Ten) {
    using namespace pstore::exchange::export_ns::details;
    base10storage<TypeParam> out;
    std::pair<char const *, char const *> const x = u2s (TypeParam{10}, &out);
    EXPECT_EQ ((std::string{x.first, x.second}), "10");
}

TYPED_TEST_P (UnsignedToString, Max) {
    using namespace pstore::exchange::export_ns::details;
    base10storage<TypeParam> out;
    auto const max = std::numeric_limits<TypeParam>::max ();
    std::pair<char const *, char const *> const x = u2s (max, &out);
    EXPECT_EQ ((std::string{x.first, x.second}), std::to_string (max));
}

REGISTER_TYPED_TEST_SUITE_P (UnsignedToString, Zero, One, Ten, Max);
using UnsignedTypes =
    testing::Types<unsigned char, unsigned int, unsigned long, unsigned long long>;
INSTANTIATE_TYPED_TEST_SUITE_P (U2S, UnsignedToString, UnsignedTypes, );
