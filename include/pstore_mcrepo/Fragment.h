//*  _____                                     _    *
//* |  ___| __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_ | '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _|| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_|  |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                 |___/                           *
//===- include/pstore_mcrepo/Fragment.h -----------------------------------===//
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
#ifndef PSTORE_MCREPO_FRAGMENT_H
#define PSTORE_MCREPO_FRAGMENT_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <type_traits>
#include <vector>

#include "pstore/address.hpp"
#include "pstore/file_header.hpp"
#include "pstore/transaction.hpp"
#include "pstore_mcrepo/Aligned.h"
#include "pstore_mcrepo/SparseArray.h"
#include "pstore_support/small_vector.hpp"

namespace pstore {
namespace repo {

//*  ___     _                     _ ___ _                *
//* |_ _|_ _| |_ ___ _ _ _ _  __ _| | __(_)_ ___  _ _ __  *
//*  | || ' \  _/ -_) '_| ' \/ _` | | _|| \ \ / || | '_ \ *
//* |___|_||_\__\___|_| |_||_\__,_|_|_| |_/_\_\\_,_| .__/ *
//*                                                |_|    *
struct InternalFixup {
  std::uint8_t Section;
  std::uint8_t Type;
  std::uint16_t Padding;
  std::uint32_t Offset;
  std::uint32_t Addend;
};

static_assert(std::is_standard_layout<InternalFixup>::value,
              "InternalFixup must satisfy StandardLayoutType");

static_assert(offsetof(InternalFixup, Section) == 0,
              "Section offset differs from expected value");
static_assert(offsetof(InternalFixup, Type) == 1,
              "Type offset differs from expected value");
static_assert(offsetof(InternalFixup, Padding) == 2,
              "Padding offset differs from expected value");
static_assert(offsetof(InternalFixup, Offset) == 4,
              "Offset offset differs from expected value");
static_assert(offsetof(InternalFixup, Addend) == 8,
              "Addend offset differs from expected value");
static_assert(sizeof(InternalFixup) == 12,
              "InternalFixup size does not match expected");

//*  ___     _                     _ ___ _                *
//* | __|_ _| |_ ___ _ _ _ _  __ _| | __(_)_ ___  _ _ __  *
//* | _|\ \ /  _/ -_) '_| ' \/ _` | | _|| \ \ / || | '_ \ *
//* |___/_\_\\__\___|_| |_||_\__,_|_|_| |_/_\_\\_,_| .__/ *
//*                                                |_|    *
struct ExternalFixup {
  pstore::address Name;
  std::uint8_t Type;
  // FIXME: much padding here.
  std::uint64_t Offset;
  std::uint64_t Addend;
};

static_assert(std::is_standard_layout<ExternalFixup>::value,
              "ExternalFixup must satisfy StandardLayoutType");
static_assert(offsetof(ExternalFixup, Name) == 0,
              "Name offset differs from expected value");
static_assert(offsetof(ExternalFixup, Type) == 8,
              "Type offset differs from expected value");
static_assert(offsetof(ExternalFixup, Offset) == 16,
              "Offset offset differs from expected value");
static_assert(offsetof(ExternalFixup, Addend) == 24,
              "Addend offset differs from expected value");
// static_assert (offsetof (external_fixup, padding3) == 20, "padding3 offset
// differs from expected value");
static_assert(sizeof(ExternalFixup) == 32,
              "ExternalFixup size does not match expected");

//*  ___         _   _           *
//* / __| ___ __| |_(_)___ _ _   *
//* \__ \/ -_) _|  _| / _ \ ' \  *
//* |___/\___\__|\__|_\___/_||_| *
//*                              *
class Section {
  friend struct SectionCheck;

public:
  /// Describes the three members of a Section as three pairs of iterators: one
  /// each for the data, internal fixups, and external fixups ranges.
  template <typename DataRangeType, typename IFixupRangeType,
            typename XFixupRangeType>
  struct Sources {
    DataRangeType DataRange;
    IFixupRangeType IfixupsRange;
    XFixupRangeType XfixupsRange;
  };

  template <typename DataRange, typename IFixupRange, typename XFixupRange>
  static inline auto makeSources(DataRange const &D, IFixupRange const &I,
                                 XFixupRange const &X)
      -> Sources<DataRange, IFixupRange, XFixupRange> {
    return {D, I, X};
  }

  template <typename DataRange, typename IFixupRange, typename XFixupRange>
  Section(DataRange const &D, IFixupRange const &I, XFixupRange const &X);

  template <typename DataRange, typename IFixupRange, typename XFixupRange>
  Section(Sources<DataRange, IFixupRange, XFixupRange> const &Src)
      : Section(Src.DataRange, Src.IfixupsRange, Src.XfixupsRange) {}

  Section(Section const &) = delete;
  Section &operator=(Section const &) = delete;
  Section(Section &&) = delete;
  Section &operator=(Section &&) = delete;

  /// A simple wrapper around the elements of one of the three arrays that make
  /// up a section. Enables the use of standard algorithms as well as
  /// range-based for loops on these collections.
  template <typename ValueType> class Container {
  public:
    using value_type = ValueType const;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = ValueType const &;
    using const_reference = reference;
    using pointer = ValueType const *;
    using const_pointer = pointer;
    using iterator = const_pointer;
    using const_iterator = iterator;

    Container(const_pointer Begin, const_pointer End)
        : Begin_{Begin}, End_{End} {
      assert(End >= Begin);
    }
    iterator begin() const { return Begin_; }
    iterator end() const { return End_; }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    size_type size() const { return End_ - Begin_; }

  private:
    const_pointer Begin_;
    const_pointer End_;
  };

  Container<std::uint8_t> data() const {
    auto Begin = alignedPtr<std::uint8_t>(this + 1);
    return {Begin, Begin + DataSize_};
  }
  Container<InternalFixup> ifixups() const {
    auto Begin = alignedPtr<InternalFixup>(data().end());
    return {Begin, Begin + NumIfixups_};
  }
  Container<ExternalFixup> xfixups() const {
    auto Begin = alignedPtr<ExternalFixup>(ifixups().end());
    return {Begin, Begin + NumXfixups_};
  }

  ///@{
  /// \brief A group of member functions which return the number of bytes
  /// occupied by a fragment instance.

  /// \returns The number of bytes occupied by this fragment section.
  std::size_t sizeBytes() const;

  /// \returns The number of bytes needed to accommodate a fragment section with
  /// the given number of data bytes and fixups.
  static std::size_t sizeBytes(std::size_t DataSize, std::size_t NumIfixups,
                               std::size_t NumXfixups);

  template <typename DataRange, typename IFixupRange, typename XFixupRange>
  static std::size_t sizeBytes(DataRange const &D, IFixupRange const &I,
                               XFixupRange const &X);

  template <typename DataRange, typename IFixupRange, typename XFixupRange>
  static std::size_t
  sizeBytes(Sources<DataRange, IFixupRange, XFixupRange> const &Src) {
    return sizeBytes(Src.DataRange, Src.IfixupsRange, Src.XfixupsRange);
  }
  ///@}

private:
  std::uint32_t NumIfixups_ = 0;
  std::uint32_t NumXfixups_ = 0;
  std::uint64_t DataSize_ = 0;

  /// A helper function which returns the distance between two iterators,
  /// clamped to the maximum range of IntType.
  template <typename IntType, typename Iterator>
  static IntType setSize(Iterator First, Iterator Last);

  /// Calculates the size of a region in the section including any necessary
  /// preceeding alignment bytes.
  /// \param Pos The starting offset within the section.
  /// \param Num The number of instance of type Ty.
  /// \returns Number of bytes occupied by the elements.
  template <typename Ty>
  static inline std::size_t partSizeBytes(std::size_t Pos, std::size_t Num) {
    if (Num > 0) {
      Pos = aligned<Ty>(Pos) + Num * sizeof(Ty);
    }
    return Pos;
  }
};

static_assert(std::is_standard_layout<Section>::value,
              "Section must satisfy StandardLayoutType");

/// A trivial class which exists solely to verify the layout of the Section type
/// (including its private member variables and is declared as a friend of it.
struct SectionCheck {
  static_assert(offsetof(Section, NumIfixups_) == 0,
                "NumIfixups_ offset differs from expected value");
  static_assert(offsetof(Section, NumXfixups_) == 4,
                "NumXfixups_ offset differs from expected value");
  static_assert(offsetof(Section, DataSize_) == 8,
                "DataSize_ offset differs from expected value");
  static_assert(sizeof(Section) == 16, "Section size does not match expected");
};

// (ctor)
// ~~~~~~
template <typename DataRange, typename IFixupRange, typename XFixupRange>
Section::Section(DataRange const &D, IFixupRange const &I,
                 XFixupRange const &X) {
  auto const Start = reinterpret_cast<std::uint8_t *>(this);
  auto P = reinterpret_cast<std::uint8_t *>(this + 1);

  if (D.first != D.second) {
    P = std::copy(D.first, D.second, alignedPtr<std::uint8_t>(P));
    DataSize_ = Section::setSize<decltype(DataSize_)>(D.first, D.second);
  }
  if (I.first != I.second) {
    P = reinterpret_cast<std::uint8_t *>(
        std::copy(I.first, I.second, alignedPtr<InternalFixup>(P)));
    NumIfixups_ = Section::setSize<decltype(NumIfixups_)>(I.first, I.second);
  }
  if (X.first != X.second) {
    P = reinterpret_cast<std::uint8_t *>(
        std::copy(X.first, X.second, alignedPtr<ExternalFixup>(P)));
    NumXfixups_ = Section::setSize<decltype(NumXfixups_)>(X.first, X.second);
  }
  assert(P >= Start &&
         static_cast<std::size_t>(P - Start) == sizeBytes(D, I, X));
}

// setSize
// ~~~~~~~
template <typename IntType, typename Iterator>
inline IntType Section::setSize(Iterator First, Iterator Last) {
  static_assert(std::is_unsigned<IntType>::value, "IntType must be unsigned");
  auto const Size = std::distance(First, Last);
  assert(Size >= 0);

  auto const USize =
      static_cast<typename std::make_unsigned<decltype(Size)>::type>(Size);
  assert(USize >= std::numeric_limits<IntType>::min());
  assert(USize <= std::numeric_limits<IntType>::max());

  return static_cast<IntType>(Size);
}

// SizeBytes
// ~~~~~~~~~
template <typename DataRange, typename IFixupRange, typename XFixupRange>
std::size_t Section::sizeBytes(DataRange const &D, IFixupRange const &I,
                               XFixupRange const &X) {
  auto const DataSize = std::distance(D.first, D.second);
  auto const NumIfixups = std::distance(I.first, I.second);
  auto const NumXfixups = std::distance(X.first, X.second);
  assert(DataSize >= 0 && NumIfixups >= 0 && NumXfixups >= 0);
  return sizeBytes(static_cast<std::size_t>(DataSize),
                   static_cast<std::size_t>(NumIfixups),
                   static_cast<std::size_t>(NumXfixups));
}

// FIXME: the members of this collection are drawn from
// RepoObjectWriter::writeRepoSectionData(). Not sure it's correct.
#define PSTORE_REPO_SECTION_TYPES \
  X(BSS)                   \
  X(Common)                \
  X(Data)                  \
  X(RelRo)                 \
  X(Text)                  \
  X(Mergeable1ByteCString) \
  X(Mergeable2ByteCString) \
  X(Mergeable4ByteCString) \
  X(MergeableConst4)       \
  X(MergeableConst8)       \
  X(MergeableConst16)      \
  X(MergeableConst32)      \
  X(MergeableConst)        \
  X(ReadOnly)              \
  X(ThreadBSS)             \
  X(ThreadData)            \
  X(ThreadLocal)           \
  X(Metadata)

#define X(a) a,
enum class SectionType : std::uint8_t { PSTORE_REPO_SECTION_TYPES };
#undef X

struct SectionContent {
  SectionContent () = default;
  SectionContent (SectionContent const & ) = default;
  SectionContent (SectionContent && ) = default;
  SectionContent & operator=(SectionContent const & ) = default;
  SectionContent & operator=(SectionContent && ) = default;

  template <typename Iterator> using Range = std::pair<Iterator, Iterator>;

  template <typename Iterator>
  static inline auto makeRange(Iterator Begin, Iterator End)
      -> Range<Iterator> {
    return {Begin, End};
  }

  explicit SectionContent(SectionType St) : Type{St} {}

  SectionType Type;
  small_vector<char, 128> Data;
  std::vector<InternalFixup> Ifixups;
  std::vector<ExternalFixup> Xfixups;

  auto makeSources() const
      -> Section::Sources<Range<decltype(Data)::const_iterator>,
                          Range<decltype(Ifixups)::const_iterator>,
                          Range<decltype(Xfixups)::const_iterator>> {

    return Section::makeSources(
        makeRange(std::begin(Data), std::end(Data)),
        makeRange(std::begin(Ifixups), std::end(Ifixups)),
        makeRange(std::begin(Xfixups), std::end(Xfixups)));
  }
};

namespace details {

/// An iterator adaptor which produces a value_type of 'SectionType const' from
/// values deferences from the supplied underlying iterator.
template <typename Iterator> class ContentTypeIterator {
public:
  using value_type = SectionType const;
  using difference_type =
      typename std::iterator_traits<Iterator>::difference_type;
  using pointer = value_type *;
  using reference = value_type &;
  using iterator_category = std::input_iterator_tag;

  ContentTypeIterator() : It_{} {}
  explicit ContentTypeIterator(Iterator It) : It_{It} {}
  ContentTypeIterator(ContentTypeIterator const &Rhs) : It_{Rhs.It_} {}
  ContentTypeIterator &operator=(ContentTypeIterator const &Rhs) {
    It_ = Rhs.It_;
    return *this;
  }
  bool operator==(ContentTypeIterator const &Rhs) const {
    return It_ == Rhs.It_;
  }
  bool operator!=(ContentTypeIterator const &Rhs) const {
    return !(operator==(Rhs));
  }
  ContentTypeIterator &operator++() {
    ++It_;
    return *this;
  }
  ContentTypeIterator operator++(int) {
    ContentTypeIterator Old{*this};
    It_++;
    return Old;
  }

  reference operator*() const { return (*It_).Type; }
  pointer operator->() const { return &(*It_).Type; }
  reference operator[](difference_type n) const { return It_[n].Type; }

private:
  Iterator It_;
};

template <typename Iterator>
inline ContentTypeIterator<Iterator> makeContentTypeIterator(Iterator It) {
  return ContentTypeIterator<Iterator>(It);
}

/// An iterator adaptor which produces a value_type of dereferences the
/// value_type of the wrapped iterator.
template <typename Iterator> class SectionContentIterator {
public:
  using value_type = typename std::pointer_traits<
      typename std::pointer_traits<Iterator>::element_type>::element_type;
  using difference_type =
      typename std::iterator_traits<Iterator>::difference_type;
  using pointer = value_type *;
  using reference = value_type &;
  using iterator_category = std::input_iterator_tag;

  SectionContentIterator() : It_{} {}
  explicit SectionContentIterator(Iterator It) : It_{It} {}
  SectionContentIterator(SectionContentIterator const &Rhs) : It_{Rhs.It_} {}
  SectionContentIterator &operator=(SectionContentIterator const &Rhs) {
    It_ = Rhs.It_;
    return *this;
  }
  bool operator==(SectionContentIterator const &Rhs) const {
    return It_ == Rhs.It_;
  }
  bool operator!=(SectionContentIterator const &Rhs) const {
    return !(operator==(Rhs));
  }
  SectionContentIterator &operator++() {
    ++It_;
    return *this;
  }
  SectionContentIterator operator++(int) {
    SectionContentIterator Old{*this};
    It_++;
    return Old;
  }

  reference operator*() const { return **It_; }
  pointer operator->() const { return &(**It_); }
  reference operator[](difference_type n) const { return *(It_[n]); }

private:
  Iterator It_;
};

template <typename Iterator>
inline SectionContentIterator<Iterator>
makeSectionContentIterator(Iterator It) {
  return SectionContentIterator<Iterator>(It);
}

} // end namespace details

//*  ___                            _    *
//* | __| _ __ _ __ _ _ __  ___ _ _| |_  *
//* | _| '_/ _` / _` | '  \/ -_) ' \  _| *
//* |_||_| \__,_\__, |_|_|_\___|_||_\__| *
//*             |___/                    *
class Fragment {
public:
  struct Deleter {
    void operator()(void *P);
  };

  template <typename TransactionType, typename Iterator>
  static auto alloc (TransactionType & transaction,
                     Iterator First, Iterator Last) -> pstore::record;

  template <typename Iterator>
  void populate (Iterator First, Iterator Last) {
    // Point past the end of the sparse array.
    auto Out = reinterpret_cast<std::uint8_t *>(this) +
               Arr_.size_bytes();

    // Copy the contents of each of the segments to the fragment.
    std::for_each(First, Last, [&Out, this](SectionContent const &C) {
      Out = reinterpret_cast<std::uint8_t *>(alignedPtr<Section>(Out));
      auto Scn = new (Out) Section(C.makeSources());
      auto Offset = reinterpret_cast<std::uintptr_t>(Scn) -
                    reinterpret_cast<std::uintptr_t>(this);
      Arr_[static_cast<unsigned>(C.Type)] = Offset;
      Out += Scn->sizeBytes();
    });
  }

  using MemberArray = SparseArray<std::uint64_t>;

  Section const &operator[](SectionType Key) const;
  std::size_t numSections() const { return Arr_.size(); }
  MemberArray const &sections() const { return Arr_; }

  /// Returns the number of bytes of storage that is required for a fragment containing
  /// the sections defined by [first, last).
  template <typename Iterator>
  static std::size_t sizeBytes (Iterator First, Iterator Last) {
    auto const NumSections = std::distance(First, Last);
    assert(NumSections >= 0);

    // Space needed by the section offset array.
    std::size_t SizeBytes = decltype(Fragment::Arr_)::size_bytes(NumSections);
    // Now the storage for each of the sections
    std::for_each(First, Last, [&SizeBytes](SectionContent const &C) {
      SizeBytes = aligned<Section>(SizeBytes);
      SizeBytes += Section::sizeBytes(C.makeSources());
    });
    return SizeBytes;
  }

private:
  template <typename IteratorIdx>
  Fragment(IteratorIdx FirstIndex, IteratorIdx LastIndex)
      : Arr_(FirstIndex, LastIndex) {}

  Section const & offsetToSection (std::uint64_t Offset) const {
    auto Ptr = reinterpret_cast<std::uint8_t const *>(this) + Offset;
    assert(reinterpret_cast<std::uintptr_t>(Ptr) % alignof(Section) == 0);
    return *reinterpret_cast<Section const *>(Ptr);
  }

  MemberArray Arr_;
};


// operator<<
// ~~~~~~~~~~
#define X(a) case (SectionType::a) : Name=#a ; break;
template <typename OStream>
OStream &operator<<(OStream &OS, SectionType T) {
  char const *Name = "*unknown*";
  switch (T) {
  PSTORE_REPO_SECTION_TYPES
  }
  return OS << Name;
}
#undef X

template <typename OStream>
OStream &operator<<(OStream &OS, InternalFixup const &Ifx) {
  using TypeT = typename std::common_type <unsigned, decltype (Ifx.Type)>::type;
  return OS << "{section:" << static_cast<unsigned>(Ifx.Section)
            << ",type:" << static_cast<TypeT>(Ifx.Type)
            << ",offset:" << Ifx.Offset << ",addend:" << Ifx.Addend << "}";
}

template <typename OStream>
OStream &operator<<(OStream &OS, ExternalFixup const &Xfx) {
  using TypeT = typename std::common_type <unsigned, decltype (Xfx.Type)>::type;
  return OS << R"({name:")" << Xfx.Name.absolute () << R"(",type:)" << static_cast <TypeT> (Xfx.Type)
            << ",offset:" << Xfx.Offset << ",addend:" << Xfx.Addend << "}";
}

template <typename OStream>
OStream &operator<<(OStream &OS, Section const &Scn) {
        auto digit_to_hex = [] (unsigned v) {
            assert (v < 0x10);
            return static_cast<char> (v + ((v < 10) ? '0' : 'a' - 10));
        };

  char const *Indent = "\n  ";
  OS << '{' << Indent << "data: ";
  for (uint8_t V : Scn.data()) {
    OS << "0x" << digit_to_hex (V >> 4) << digit_to_hex (V & 0xF)<< ',';
  }
  OS << Indent << "ifixups: [ ";
  for (auto const &Ifixup : Scn.ifixups()) {
    OS << Ifixup << ", ";
  }
  OS << ']' << Indent << "xfixups: [ ";
  for (auto const &Xfixup : Scn.xfixups()) {
    OS << Xfixup << ", ";
  }
  return OS << "]\n}";
}

template <typename OStream>
OStream &operator<<(OStream &OS, Fragment const &Frag) {
  for (auto const Key : Frag.sections().getIndices()) {
    auto const Type = static_cast<SectionType>(Key);
    OS << Type << ": " << Frag[Type] << '\n';
  }
  return OS;
}

template <typename TransactionType, typename Iterator>
auto Fragment::alloc (TransactionType & Transaction,
                      Iterator First, Iterator Last) -> pstore::record {
  static_assert(
      (std::is_same<typename std::iterator_traits<Iterator>::value_type,
                    SectionContent>::value),
      "Iterator value_type should be SectionContent");

  // Compute the number of bytes of storage that we'll need for this fragment.
  auto Size = Fragment::sizeBytes (First, Last);

  // Allocate storage for the fragment including its three arrays.
  std::pair <std::shared_ptr<void>, pstore::address> Storage = Transaction.alloc_rw(Size, alignof (Fragment));
  auto Ptr = Storage.first;

  // Construct the basic fragment structure into this memory.
  auto FragmentPtr =
      new (Ptr.get()) Fragment(details::makeContentTypeIterator(First),
                               details::makeContentTypeIterator(Last));
  // Point past the end of the sparse array.
  auto Out = reinterpret_cast<std::uint8_t *>(FragmentPtr) +
             FragmentPtr->Arr_.size_bytes();

  // Copy the contents of each of the segments to the fragment.
  std::for_each(First, Last, [&Out, FragmentPtr](SectionContent const &C) {
    Out = reinterpret_cast<std::uint8_t *>(alignedPtr<Section>(Out));
    auto Scn = new (Out) Section(C.makeSources());
    auto Offset = reinterpret_cast<std::uintptr_t>(Scn) -
                  reinterpret_cast<std::uintptr_t>(FragmentPtr);
    FragmentPtr->Arr_[static_cast<unsigned>(C.Type)] = Offset;
    Out += Scn->sizeBytes();
  });

  assert(Out >= Ptr.get() && static_cast<std::size_t>(Out - reinterpret_cast <std::uint8_t *> (Ptr.get())) == Size);
  return {Storage.second, Size};
}

} // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_FRAGMENT_H
// eof: include/pstore_mcrepo/Fragment.h
