//*   __                                      _    *
//*  / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                |___/                           *
//===- unittests/pstore_mcrepo/test_fragment.cpp --------------------------===//
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

#include "pstore_mcrepo/Fragment.h"
#include "gmock/gmock.h"
#include <memory>
#include <vector>

using namespace pstore::repo;


namespace {
    class transaction {
    public:
        auto alloc_rw (std::size_t size, unsigned /*align*/) -> std::pair<std::shared_ptr<void>, pstore::address> {
            auto ptr = std::shared_ptr <std::uint8_t> (new std::uint8_t [size], [] (std::uint8_t * p) { delete [] p; });
            storage.push_back (ptr);

            static_assert (sizeof (std::uint8_t *) >= sizeof (pstore::address), "expected address to be no larger than a pointer");
            return std::make_pair (ptr, pstore::address {reinterpret_cast <std::uintptr_t> (ptr.get ())});
        }

        std::vector <std::shared_ptr <std::uint8_t>> storage;
    };

    class FragmentTest : public ::testing::Test {
    protected:
        transaction transaction_;
    };
}

TEST_F(FragmentTest, Empty) {
    std::vector<SectionContent> C;
    pstore::record Record = Fragment::alloc (transaction_, std::begin(C), std::end(C));
    auto F = reinterpret_cast <Fragment const *> (Record.addr.absolute ());

    assert (transaction_.storage.back ().get () == reinterpret_cast <std::uint8_t const *> (F));
    EXPECT_EQ(0U, F->numSections ());
}

TEST_F (FragmentTest, MakeReadOnlySection) {
    SectionContent RoData{SectionType::ReadOnly};
    RoData.Data.assign({'r', 'o', 'd', 'a', 't', 'a'});

    std::vector<SectionContent> c{RoData};
    auto Record = Fragment::alloc (transaction_, std::begin(c), std::end(c));

    assert (transaction_.storage.back ().get () == reinterpret_cast <std::uint8_t const *> (Record.addr.absolute ()));
    auto F = reinterpret_cast <Fragment const *> (transaction_.storage.back ().get ());


    std::vector<std::size_t> const Expected{
      static_cast<std::size_t>(SectionType::ReadOnly)};
    auto Indices = F->sections ().getIndices();
    std::vector<std::size_t> Actual(std::begin(Indices), std::end(Indices));
    EXPECT_THAT(Actual, ::testing::ContainerEq(Expected));

    Section const &S = (*F)[SectionType::ReadOnly];
    auto DataBegin = std::begin(S.data());
    auto DataEnd = std::end(S.data());
    auto RoDataBegin = std::begin(RoData.Data);
    auto RoDataEnd = std::end(RoData.Data);
    ASSERT_EQ (std::distance (DataBegin, DataEnd), std::distance (RoDataBegin, RoDataEnd));
    EXPECT_TRUE(std::equal(DataBegin, DataEnd, RoDataBegin));
    EXPECT_EQ(0U, S.ifixups().size());
    EXPECT_EQ(0U, S.xfixups().size());
}

// eof: unittests/pstore_mcrepo/test_fragment.cpp
