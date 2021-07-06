//===- unittests/mcrepo/test_fragment.cpp ---------------------------------===//
//*   __                                      _    *
//*  / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                |___/                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/mcrepo/fragment.hpp"

// Standard library includes
#include <memory>
#include <vector>

// 3rd party
#include <gmock/gmock.h>

// Local includes
#include "transaction.hpp"

using namespace pstore::repo;

namespace {

    class FragmentTest : public ::testing::Test {
    protected:
        transaction transaction_;

        using string_address = pstore::typed_address<pstore::indirect_string>;
        static constexpr string_address indirect_string_address (std::uint64_t x) {
            return string_address{pstore::address{x}};
        }
    };

    template <typename Iterator>
    auto build_sections (Iterator begin, Iterator end)
        -> std::vector<std::unique_ptr<section_creation_dispatcher>> {
        static_assert ((std::is_same<typename std::iterator_traits<Iterator>::value_type,
                                     section_content>::value),
                       "Iterator value_type must be section_content pointer");
        PSTORE_ASSERT (std::distance (begin, end) >= 0);

        std::vector<std::unique_ptr<section_creation_dispatcher>> dispatchers;
        std::for_each (begin, end, [&dispatchers] (section_content const & section) {
            dispatchers.emplace_back (
                new generic_section_creation_dispatcher (section.kind, &section));
        });
        return dispatchers;
    }

    template <typename Iterator>
    pstore::extent<fragment> build_fragment (transaction & transaction, Iterator begin,
                                             Iterator end) {
        std::vector<std::unique_ptr<section_creation_dispatcher>> fdata =
            build_sections (begin, end);
        return fragment::alloc (transaction, pstore::make_pointee_adaptor (fdata.begin ()),
                                pstore::make_pointee_adaptor (fdata.end ()));
    }

    template <typename SectionIterator, typename DefinitionIterator,
              typename = typename std::enable_if_t<
                  std::is_same<typename std::iterator_traits<DefinitionIterator>::value_type,
                               pstore::repo::linked_definitions::value_type>::value>>
    pstore::extent<fragment>
    build_fragment (transaction & transaction, SectionIterator section_begin,
                    SectionIterator section_end, DefinitionIterator definition_begin,
                    DefinitionIterator definition_end) {
        PSTORE_ASSERT (std::distance (definition_begin, definition_end) > 0);

        std::vector<std::unique_ptr<section_creation_dispatcher>> dispatchers =
            build_sections (section_begin, section_end);

        dispatchers.emplace_back (
            new linked_definitions_creation_dispatcher (definition_begin, definition_end));

        return fragment::alloc (transaction, pstore::make_pointee_adaptor (dispatchers.begin ()),
                                pstore::make_pointee_adaptor (dispatchers.end ()));
    }

} // anonymous namespace

TEST_F (FragmentTest, Empty) {
    std::vector<std::unique_ptr<section_creation_dispatcher>> c;
    auto extent = fragment::alloc (transaction_, pstore::make_pointee_adaptor (std::begin (c)),
                                   pstore::make_pointee_adaptor (std::end (c)));

    auto f = reinterpret_cast<fragment const *> (extent.addr.absolute ());
    PSTORE_ASSERT (transaction_.get_storage ().begin ()->first ==
                   reinterpret_cast<std::uint8_t const *> (f));
    EXPECT_EQ (0U, f->size ());
}

TEST_F (FragmentTest, MakeReadOnlySection) {
    std::array<section_content, 1> c = {{{section_kind::read_only, std::uint8_t{4} /*alignment*/}}};
    section_content & rodata = c.back ();
    rodata.data.assign ({'r', 'o', 'd', 'a', 't', 'a'});
    auto extent = build_fragment (transaction_, std::begin (c), std::end (c));

    ASSERT_EQ (transaction_.get_storage ().begin ()->first,
               reinterpret_cast<std::uint8_t const *> (extent.addr.absolute ()));
    auto f = reinterpret_cast<fragment const *> (transaction_.get_storage ().begin ()->first);

    std::vector<section_kind> const expected{section_kind::read_only};
    auto const indices = f->members ().get_indices ();
    std::vector<section_kind> const actual{std::begin (indices), std::end (indices)};
    EXPECT_THAT (actual, ::testing::ContainerEq (expected));

    generic_section const & s = f->at<section_kind::read_only> ();

    EXPECT_EQ (4U, section_alignment (s));
    EXPECT_EQ (6U, section_size (s));

    auto data_begin = std::begin (s.payload ());
    auto data_end = std::end (s.payload ());
    auto rodata_begin = std::begin (rodata.data);
    auto rodata_end = std::end (rodata.data);
    ASSERT_EQ (std::distance (data_begin, data_end), std::distance (rodata_begin, rodata_end));
    EXPECT_TRUE (std::equal (data_begin, data_end, rodata_begin));
    EXPECT_EQ (4U, s.align ());
    EXPECT_EQ (0U, s.ifixups ().size ());
    EXPECT_EQ (0U, s.xfixups ().size ());
    EXPECT_EQ (f->atp<section_kind::linked_definitions> (), nullptr);
}

TEST_F (FragmentTest, MakeTextSectionWithFixups) {
    using ::testing::ElementsAre;
    using ::testing::ElementsAreArray;
    using section_kind = pstore::repo::section_kind;

    std::vector<std::uint8_t> const original{'t', 'e', 'x', 't'};

    std::array<section_content, 1> c = {{{section_kind::text, std::uint8_t{16} /*alignment*/}}};
    {
        // Build the text section's contents and fixups.
        section_content & text = c.back ();
        text.data.assign (std::begin (original), std::end (original));
        text.ifixups.emplace_back (internal_fixup{section_kind::text, 1, 1, 1});
        text.ifixups.emplace_back (internal_fixup{section_kind::data, 2, 2, 2});
        text.xfixups.emplace_back (external_fixup{indirect_string_address (3), relocation_type{3},
                                                  pstore::repo::reference_strength::strong,
                                                  UINT64_C (3) /*offset*/, INT64_C (3) /*addend*/});
        text.xfixups.emplace_back (external_fixup{indirect_string_address (4), relocation_type{4},
                                                  pstore::repo::reference_strength::weak,
                                                  UINT64_C (4) /*offset*/, INT64_C (4) /*addend*/});
        text.xfixups.emplace_back (external_fixup{indirect_string_address (5), relocation_type{5},
                                                  pstore::repo::reference_strength::strong,
                                                  UINT64_C (5) /*offset*/, INT64_C (5) /*addend*/});
    }

    auto extent = build_fragment (transaction_, std::begin (c), std::end (c));

    ASSERT_EQ (transaction_.get_storage ().begin ()->first,
               reinterpret_cast<std::uint8_t const *> (extent.addr.absolute ()));


    auto f = reinterpret_cast<fragment const *> (transaction_.get_storage ().begin ()->first);


    std::vector<section_kind> const expected{section_kind::text};
    auto const indices = f->members ().get_indices ();
    std::vector<section_kind> actual{std::begin (indices), std::end (indices)};
    EXPECT_THAT (actual, ::testing::ContainerEq (expected));

    generic_section const & s = f->at<section_kind::text> ();

    EXPECT_EQ (16U, section_alignment (s));
    EXPECT_EQ (4U, section_size (s));

    EXPECT_EQ (16U, s.align ());
    EXPECT_EQ (4U, s.payload ().size ());
    EXPECT_EQ (4U, s.size ());
    EXPECT_EQ (2U, s.ifixups ().size ());
    EXPECT_EQ (3U, s.xfixups ().size ());

    EXPECT_THAT (s.payload (), ElementsAreArray (original));
    EXPECT_THAT (s.ifixups (), ElementsAre (internal_fixup{section_kind::text, 1, 1, 1},
                                            internal_fixup{section_kind::data, 2, 2, 2}));
    EXPECT_THAT (s.xfixups (),
                 ElementsAre (external_fixup{indirect_string_address (3), relocation_type{3},
                                             pstore::repo::reference_strength::strong,
                                             UINT64_C (3) /*offset*/, INT64_C (3) /*addend*/},
                              external_fixup{indirect_string_address (4), relocation_type{4},
                                             pstore::repo::reference_strength::weak,
                                             UINT64_C (4) /*offset*/, INT64_C (4) /*addend*/},
                              external_fixup{indirect_string_address (5), relocation_type{5},
                                             pstore::repo::reference_strength::strong,
                                             UINT64_C (5) /*offset*/, INT64_C (5) /*addend*/}));
}

TEST_F (FragmentTest, MakeTextSectionWithLinkedDefinitions) {
    using ::testing::ElementsAre;
    using ::testing::ElementsAreArray;

    std::vector<section_content> c;

    constexpr linked_definitions::value_type ld{pstore::index::digest{0, 0xffff}, 17U,
                                                pstore::typed_address<definition>::make (37U)};
    std::vector<linked_definitions::value_type> d{ld};

    auto extent = build_fragment (transaction_, std::begin (c), std::end (c), d.data (),
                                  d.data () + d.size ());

    ASSERT_EQ (transaction_.get_storage ().begin ()->first,
               reinterpret_cast<std::uint8_t const *> (extent.addr.absolute ()));
    auto f = reinterpret_cast<fragment const *> (transaction_.get_storage ().begin ()->first);

    std::vector<section_kind> const contents (f->begin (), f->end ());
    EXPECT_THAT (contents, ::testing::ElementsAre (section_kind::linked_definitions));

    pstore::repo::linked_definitions const * const linked_definitions =
        f->atp<section_kind::linked_definitions> ();
    ASSERT_NE (linked_definitions, nullptr);

    EXPECT_EQ (1U, section_alignment (*linked_definitions));
    EXPECT_EQ (0U, section_size (*linked_definitions));

    EXPECT_EQ (1U, linked_definitions->size ());
    EXPECT_EQ ((*linked_definitions)[0], ld);
}

namespace pstore {
    namespace repo {

        // An implementation of the gtest PrintTo function to avoid ambiguity between the test
        // harness's function and our template implementation of operator<<.
        void PrintTo (section_kind st, ::std::ostream * os);
        void PrintTo (section_kind st, ::std::ostream * os) { *os << st; }

    } // namespace repo
} // namespace pstore

namespace {

    void build_two_sections (transaction & transaction_) {
        std::array<section_content, 2> c{{
            {section_kind::read_only, std::uint8_t{1}}, // section kind, alignment
            {section_kind::thread_data, std::uint8_t{2}},
        }};
        using base_type = std::underlying_type<section_kind>::type;
        ASSERT_LT (static_cast<base_type> (c.at (0).kind), static_cast<base_type> (c.at (1).kind));

        section_content & rodata = c.at (0);
        ASSERT_EQ (rodata.kind, section_kind::read_only);
        rodata.data.assign ({'r', 'o', 'd', 'a', 't', 'a'});

        section_content & tls = c.at (1);
        EXPECT_EQ (tls.kind, section_kind::thread_data);
        tls.data.assign ({'t', 'l', 's'});

        auto const extent = build_fragment (transaction_, std::begin (c), std::end (c));

        ASSERT_EQ (transaction_.get_storage ().begin ()->first,
                   reinterpret_cast<std::uint8_t const *> (extent.addr.absolute ()));
    }

} // end anonymous namespace

TEST_F (FragmentTest, TwoSections) {
    build_two_sections (transaction_);

    auto f = reinterpret_cast<fragment const *> (transaction_.get_storage ().begin ()->first);
    std::vector<section_kind> const expected{section_kind::read_only, section_kind::thread_data};
    auto const indices = f->members ().get_indices ();
    std::vector<section_kind> const actual{std::begin (indices), std::end (indices)};
    EXPECT_THAT (actual, ::testing::ContainerEq (expected));

    generic_section const & rodata = f->at<section_kind::read_only> ();
    generic_section const & tls = f->at<section_kind::thread_data> ();
    EXPECT_LT (rodata.payload ().begin (), tls.payload ().begin ());
}

TEST_F (FragmentTest, Iterator) {
    build_two_sections (transaction_);

    auto f = reinterpret_cast<fragment const *> (transaction_.get_storage ().begin ()->first);
    fragment::const_iterator begin = f->begin ();
    fragment::const_iterator end = f->end ();
    std::vector<section_kind> const contents{begin, end};
    EXPECT_THAT (contents,
                 ::testing::ElementsAre (section_kind::read_only, section_kind::thread_data));
}
