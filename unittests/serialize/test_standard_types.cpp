//*      _                  _               _   _                          *
//*  ___| |_ __ _ _ __   __| | __ _ _ __ __| | | |_ _   _ _ __   ___  ___  *
//* / __| __/ _` | '_ \ / _` |/ _` | '__/ _` | | __| | | | '_ \ / _ \/ __| *
//* \__ \ || (_| | | | | (_| | (_| | | | (_| | | |_| |_| | |_) |  __/\__ \ *
//* |___/\__\__,_|_| |_|\__,_|\__,_|_|  \__,_|  \__|\__, | .__/ \___||___/ *
//*                                                 |___/|_|               *
//===- unittests/serialize/test_standard_types.cpp ------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/serialize/standard_types.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <set>
#include <string>
#include <type_traits>

#include <gmock/gmock.h>

#include "pstore/serialize/archive.hpp"
#include "pstore/serialize/types.hpp"

using namespace std::string_literals;

namespace {

    class StringWriter : public ::testing::Test {
    public:
        StringWriter ()
                : writer_{bytes_} {}

    protected:
        std::vector<std::uint8_t> bytes_;
        pstore::serialize::archive::vector_writer writer_;

        static constexpr auto const three_byte_varint = std::string::size_type{1} << 14;
    };

} // end anonymous namespace

TEST_F (StringWriter, WriteCharString) {
    std::string const str{"hello"};
    pstore::serialize::write (writer_, str);

    EXPECT_THAT (bytes_,
                 ::testing::ElementsAre (std::uint8_t{0b1011}, std::uint8_t{0}, std::uint8_t{'h'},
                                         std::uint8_t{'e'}, std::uint8_t{'l'}, std::uint8_t{'l'},
                                         std::uint8_t{'o'}));
    EXPECT_EQ (bytes_.size (), writer_.bytes_consumed ());
    EXPECT_EQ (bytes_.size (), writer_.bytes_produced ());
}

TEST_F (StringWriter, WriteMaxTwoByteLengthCharString) {
    auto const src_length = three_byte_varint - 1;
    auto const src_char = 'a';
    std::string const str (src_length, src_char);

    pstore::serialize::write (writer_, str);

    EXPECT_EQ (str.length () + 2, bytes_.size ());
    auto first = std::begin (bytes_);
    EXPECT_EQ (0b11111110, *(first++));
    EXPECT_EQ (0b11111111, *(first++));
    std::vector<std::uint8_t> const body (first, std::end (bytes_));
    std::vector<std::uint8_t> const expected_body (src_length, src_char);

    EXPECT_THAT (body, ::testing::ContainerEq (expected_body));
    EXPECT_EQ (bytes_.size (), writer_.bytes_consumed ());
    EXPECT_EQ (bytes_.size (), writer_.bytes_produced ());
}

TEST_F (StringWriter, WriteThreeByteLengthCharString) {
    auto const src_length = three_byte_varint;
    auto const src_char = 'a';
    std::string const str (src_length, src_char);

    pstore::serialize::write (writer_, str);

    EXPECT_EQ (str.length () + 3, bytes_.size ());
    auto first = std::begin (bytes_);
    EXPECT_EQ (0b00000100, *(first++));
    EXPECT_EQ (0b00000000, *(first++));
    EXPECT_EQ (0b00000010, *(first++));
    std::vector<std::uint8_t> const body (first, std::end (bytes_));
    std::vector<std::uint8_t> const expected_body (src_length, src_char);

    EXPECT_THAT (body, ::testing::ContainerEq (expected_body));
    EXPECT_EQ (bytes_.size (), writer_.bytes_consumed ());
    EXPECT_EQ (bytes_.size (), writer_.bytes_produced ());
}

TEST_F (StringWriter, ReadCharString) {
    std::string const str{"hello"};
    pstore::serialize::write (writer_, str);

    auto archive = pstore::serialize::archive::make_reader (std::begin (writer_));
    std::string const actual = pstore::serialize::read<std::string> (archive);
    EXPECT_EQ (str, actual);
    EXPECT_EQ (std::end (writer_), archive.iterator ());
}

TEST_F (StringWriter, ReadMaxTwoByteLengthCharString) {
    auto const src_length = three_byte_varint - 1;
    auto const src_char = 'a';

    // Produce a serialized string of 'length' character 'a's.
    {
        auto output_it = std::back_inserter (bytes_);
        pstore::varint::encode (src_length, output_it);
        std::generate_n (output_it, src_length,
                         [src_char] () { return static_cast<std::uint8_t> (src_char); });
    }

    auto archive = pstore::serialize::archive::make_reader (std::begin (writer_));
    std::string const actual = pstore::serialize::read<std::string> (archive);
    std::string const expected (src_length, src_char);
    EXPECT_EQ (expected, actual);
    EXPECT_EQ (std::end (writer_), archive.iterator ());
}

TEST_F (StringWriter, ReadThreeByteLengthCharString) {
    auto const src_length = three_byte_varint;
    auto const src_char = 'a';

    // Produce a serialized string of 'length' character 'a's.
    {
        auto output_it = std::back_inserter (bytes_);
        pstore::varint::encode (src_length, output_it);
        std::generate_n (output_it, src_length,
                         [src_char] () { return static_cast<std::uint8_t> (src_char); });
    }

    auto archive = pstore::serialize::archive::make_reader (std::begin (writer_));
    std::string const actual = pstore::serialize::read<std::string> (archive);
    std::string const expected (src_length, src_char);
    EXPECT_EQ (expected, actual);
    EXPECT_EQ (std::end (writer_), archive.iterator ());
}

TEST_F (StringWriter, WriteTwoStrings) {
    std::size_t s1 = pstore::serialize::write (writer_, "a"s);
    std::size_t s2 = pstore::serialize::write (writer_, "b"s);
    EXPECT_EQ (0U, s1);
    EXPECT_EQ (3U, s2);
}


namespace {
    class SetWriter : public ::testing::Test {
    public:
        SetWriter ()
                : writer_{bytes_} {}

        virtual void SetUp () final { pstore::serialize::write (writer_, set_); }

    protected:
        static std::set<int> const set_;
        std::vector<std::uint8_t> bytes_;
        pstore::serialize::archive::vector_writer writer_;
    };

    std::set<int> const SetWriter::set_{5, 3, 2};
} // namespace

// Teach the serializer how to read and write std::set<int>
namespace pstore {
    namespace serialize {
        template <>
        struct serializer<std::set<int>> {
            using value_type = std::set<int>;

            template <typename Archive>
            static auto write (Archive && archive, value_type const & ty)
                -> archive_result_type<Archive> {
                return container_archive_helper<value_type>::write (std::forward<Archive> (archive),
                                                                    ty);
            }

            template <typename Archive>
            static void read (Archive && archive, value_type & out) {
                new (&out) value_type;
                auto inserter = [&out] (int v) { out.insert (v); };
                container_archive_helper<value_type>::read (std::forward<Archive> (archive),
                                                            inserter);
            }
        };
    } // namespace serialize
} // namespace pstore

TEST_F (SetWriter, write) {
    std::vector<std::uint8_t> bytes;
    pstore::serialize::archive::vector_writer expected{bytes};
    pstore::serialize::write (expected, std::size_t{3});
    pstore::serialize::write (expected, int{2});
    pstore::serialize::write (expected, int{3});
    pstore::serialize::write (expected, int{5});

    auto const num_expected_bytes = std::distance (std::begin (expected), std::end (expected));
    auto const num_actual_bytes = std::distance (std::begin (writer_), std::end (writer_));
    ASSERT_EQ (num_expected_bytes, num_actual_bytes);

    EXPECT_TRUE (std::equal (std::begin (expected), std::end (expected), std::begin (writer_)));
}

TEST_F (SetWriter, read) {
    auto archive = pstore::serialize::archive::make_reader (std::begin (writer_));
    auto actual = pstore::serialize::read<std::set<int>> (archive);
    static_assert (std::is_same<decltype (actual), std::set<int>>::value,
                   "expected return type of serialize::read<std::set<int>> to be std::set<int>");
    EXPECT_EQ (set_, actual);
    EXPECT_EQ (std::end (writer_), archive.iterator ());
}



namespace {
    class MapWriter : public ::testing::Test {
    public:
        using map_type = std::map<std::string, std::string>;
        MapWriter ()
                : writer_ (bytes_) {}

        virtual void SetUp () final { pstore::serialize::write (writer_, map_); }

    protected:
        static map_type const map_;
        std::vector<std::uint8_t> bytes_;
        pstore::serialize::archive::vector_writer writer_;
    };

    MapWriter::map_type const MapWriter::map_{
        {"k1", "First key"},
        {"k2", "Second key"},
    };
} // namespace

namespace pstore {
    namespace serialize {
        template <>
        struct serializer<MapWriter::map_type::value_type> {
            using value_type = MapWriter::map_type::value_type;

            template <typename Archive>
            static auto write (Archive && archive, value_type const & ty)
                -> archive_result_type<Archive> {
                auto result = serialize::write (std::forward<Archive> (archive), ty.first);
                serialize::write (std::forward<Archive> (archive), ty.second);
                return result;
            }

            template <typename Archive>
            static void read (Archive && archive, value_type & out) {
                auto const first =
                    serialize::read<decltype (value_type::first)> (std::forward<Archive> (archive));
                auto const second = serialize::read<decltype (value_type::second)> (
                    std::forward<Archive> (archive));
                new (&out) value_type (first, second);
            }
        };

        // Teach the serializer how to read and write std::map<std::string, std::string>
        template <>
        struct serializer<MapWriter::map_type> {
            using value_type = MapWriter::map_type;

            template <typename Archive>
            static auto write (Archive && archive, value_type const & ty)
                -> archive_result_type<Archive> {
                return container_archive_helper<value_type>::write (std::forward<Archive> (archive),
                                                                    ty);
            }

            template <typename Archive>
            static void read (Archive && archive, value_type & out) {
                new (&out) value_type;
                auto inserter = [&out] (MapWriter::map_type::value_type const & v) {
                    out.insert (v);
                };
                return container_archive_helper<value_type>::read (std::forward<Archive> (archive),
                                                                   inserter);
            }
        };
    } // namespace serialize
} // namespace pstore

TEST_F (MapWriter, write) {
    std::vector<std::uint8_t> expected_bytes;
    pstore::serialize::archive::vector_writer expected{expected_bytes};

    pstore::serialize::write (expected, std::size_t{2});
    pstore::serialize::write (expected, "k1"s);
    pstore::serialize::write (expected, "First key"s);
    pstore::serialize::write (expected, "k2"s);
    pstore::serialize::write (expected, "Second key"s);

    auto const num_expected_bytes = std::distance (std::begin (expected), std::end (expected));
    auto const num_actual_bytes = std::distance (std::begin (writer_), std::end (writer_));
    ASSERT_EQ (num_expected_bytes, num_actual_bytes);

    EXPECT_TRUE (std::equal (std::begin (expected), std::end (expected), std::begin (writer_)));
}

TEST_F (MapWriter, read) {
    auto archive = pstore::serialize::archive::make_reader (std::begin (writer_));
    MapWriter::map_type const actual = pstore::serialize::read<MapWriter::map_type> (archive);
    EXPECT_EQ (map_, actual);
    EXPECT_EQ (std::end (writer_), archive.iterator ());
}
