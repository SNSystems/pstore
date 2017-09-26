//*  ____                               _                          *
//* / ___| _ __   __ _ _ __ ___  ___   / \   _ __ _ __ __ _ _   _  *
//* \___ \| '_ \ / _` | '__/ __|/ _ \ / _ \ | '__| '__/ _` | | | | *
//*  ___) | |_) | (_| | |  \__ \  __// ___ \| |  | | | (_| | |_| | *
//* |____/| .__/ \__,_|_|  |___/\___/_/   \_\_|  |_|  \__,_|\__, | *
//*       |_|                                               |___/  *
//===- include/pstore_mcrepo/SparseArray.h --------------------------------===//
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
#ifndef LLVM_MCREPOFRAGMENT_SPARSE_ARRAY_H
#define LLVM_MCREPOFRAGMENT_SPARSE_ARRAY_H

#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace pstore {
namespace repo {

#ifdef _MSC_VER
// TODO: VC2017 won't allow popCount to be constexpr. Sigh.
inline unsigned popCount(unsigned char x) {
  static_assert(sizeof(unsigned char) <= sizeof(unsigned __int16),
                "Expected unsigned char to be 16 bits or less");
  return __popcnt16(x);
}
inline unsigned popCount(unsigned short x) {
  static_assert(sizeof(unsigned short) == sizeof(unsigned __int16),
                "Expected unsigned short to be 16 bits");
  return __popcnt16(x);
}
inline unsigned popCount(unsigned x) { return __popcnt(x); }
inline unsigned popCount(unsigned long x) {
  static_assert(sizeof(unsigned long) == sizeof(unsigned int),
                "Expected sizeof unsigned long to match sizeof unsigned int");
  return popCount(static_cast<unsigned int>(x));
}
inline unsigned popCount(unsigned long long x) {
  static_assert(sizeof(unsigned long long) == sizeof(unsigned __int64),
                "Expected unsigned long long to be 64 bits");
  return static_cast<unsigned>(__popcnt64(x));
}
#else
inline constexpr unsigned popCount(unsigned char x) {
  return static_cast<unsigned>(__builtin_popcount(x));
}
inline constexpr unsigned popCount(unsigned short x) {
  return static_cast<unsigned>(__builtin_popcount(x));
}
inline constexpr unsigned popCount(unsigned x) {
  return static_cast<unsigned>(__builtin_popcount(x));
}
inline constexpr unsigned popCount(unsigned long x) {
  return static_cast<unsigned>(__builtin_popcountl(x));
}
inline constexpr unsigned popCount(unsigned long long x) {
  return static_cast<unsigned>(__builtin_popcountll(x));
}
#endif

namespace details {

// TODO: this needs to implement all of the members required of a
// RandomAccessIterator.
/// \tparam Iterator  An iterator type whose value_type is a std::pair<>.
/// \tparam Accessor  The type of a function which will be called to yield the
/// values from the pair. It should accept a single parameter of type Iterator
/// and return the value of the first or second field of the referenced
/// std::pair<> instance.
template <typename Iterator, typename Accessor> class PairFieldIterator {
  using return_type = typename std::result_of<Accessor(Iterator)>::type;
  static_assert(std::is_reference<return_type>::value,
                "return type from Accessor must be a reference");

public:
  using value_type = typename std::remove_reference<return_type>::type;
  using pointer = value_type *;
  using const_pointer = value_type const *;
  using reference = value_type &;
  using const_reference = value_type const &;

  using difference_type =
      typename std::iterator_traits<Iterator>::difference_type;
  using iterator_category =
      typename std::iterator_traits<Iterator>::iterator_category;

  PairFieldIterator(Iterator It, Accessor Acc)
      : It_{std::move(It)}, Acc_{std::move(Acc)} {}

  difference_type operator-(PairFieldIterator const &Other) const {
    return It_ - Other.It_;
  }

  PairFieldIterator &operator+=(difference_type Rhs) {
    It_ += Rhs;
    return *this;
  }
  PairFieldIterator &operator-=(difference_type Rhs) {
    It_ -= Rhs;
    return *this;
  }

  PairFieldIterator operator-(difference_type Rhs) {
    PairFieldIterator res{*this};
    It_ -= Rhs;
    return res;
  }
  PairFieldIterator operator+(difference_type Rhs) {
    PairFieldIterator Res{*this};
    It_ += Rhs;
    return Res;
  }

  PairFieldIterator &operator++() {
    It_++;
    return *this;
  }
  PairFieldIterator operator++(int) {
    auto RetVal = *this;
    ++(*this);
    return RetVal;
  }
  bool operator<(PairFieldIterator const &Other) const {
    return It_ < Other.It_;
  }
  bool operator==(PairFieldIterator const &Other) const {
    return It_ == Other.It_ && Acc_ == Other.Acc_;
  }
  bool operator!=(PairFieldIterator const &Other) const {
    return !(*this == Other);
  }

  const_reference operator*() const {
    // Delegate the actual deferencing of the iterator to the accessor function.
    return Acc_(It_);
  }
  const_pointer operator->() const {
    // Delegate the actual deferencing of the iterator to the accessor function.
    // Since we statically assert that the return type of Acc_ is a reference,
    // we can take the address of its returned value.
    return &Acc_(It_);
  }

private:
  Iterator It_;
  /// An accessor function which will be used to extract one of the field
  /// members from the referenced value.
  Accessor Acc_;
};

// TODO: firstAccessor() was originally a lambda defined inside
// makePairFieldIterator() but this is rejected by VS2017.
/// Returns a reference to the 'first' field from the value produce by
/// dereferencing iterator 'It'.
template <typename Iterator>
inline auto firstAccessor(Iterator It)
    -> decltype(std::iterator_traits<Iterator>::value_type::first) const & {
  return It->first;
}

// TODO: secondAccessor() was originally a lambda defined inside
// makePairFieldIterator() but this is rejected by VS2017.
/// Returns a reference to the 'second' field from the value produce by
/// dereferencing iterator 'It'.
template <typename Iterator>
inline auto secondAccessor(Iterator It)
    -> decltype(std::iterator_traits<Iterator>::value_type::second) const & {
  return It->second;
}

template <typename Iterator, typename Accessor>
auto makePairFieldIterator(Iterator It, Accessor Acc)
    -> PairFieldIterator<Iterator, Accessor> {
  return {It, Acc};
}

} // end namespace details

template <typename ValueType, typename BitmapType = std::uint64_t>
class SparseArray {
  template <typename V, typename B>
  friend bool operator==(SparseArray<V, B> const &Lhs,
                         SparseArray<V, B> const &Rhs);

public:
  using value_type = ValueType;
  using size_type = std::size_t;
  using reference = value_type &;
  using const_reference = value_type const &;
  using iterator = ValueType *;
  using const_iterator = ValueType const *;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  SparseArray(SparseArray const &) = delete;
  SparseArray(SparseArray &&) = delete;
  SparseArray &operator=(SparseArray const &) = delete;
  SparseArray &operator=(SparseArray &&) = delete;

  ~SparseArray();

  template <typename IteratorIdx, typename IteratorV>
  static auto make_unique(IteratorIdx FirstIndex, IteratorIdx LastIndex,
                          IteratorV FirstValue, IteratorV LastValue)
      -> std::unique_ptr<SparseArray>;

  template <typename IteratorIdx>
  static auto make_unique(IteratorIdx FirstIndex, IteratorIdx LastIndex,
                          std::initializer_list<ValueType> Values)
      -> std::unique_ptr<SparseArray>;

  template <typename Iterator>
  static auto make_unique(Iterator begin, Iterator end)
      -> std::unique_ptr<SparseArray>;

  static auto make_unique(std::initializer_list<size_type> Indices,
                          std::initializer_list<ValueType> Values = {})
      -> std::unique_ptr<SparseArray>;

  static auto
  make_unique(std::initializer_list<std::pair<size_type, ValueType>> v)
      -> std::unique_ptr<SparseArray> {
    return make_unique(std::begin(v), std::end(v));
  }

  /// \name Capacity
  ///@{
  constexpr bool empty() const noexcept { return Bitmap_ == 0; }
  constexpr size_type size() const noexcept { return popCount(Bitmap_); }
  static constexpr size_type max_size() noexcept {
    return sizeof(BitmapType) * 8;
  }

  bool has_index(size_type pos) const noexcept {
    if (pos >= max_size()) {
      return false;
    }
    auto const bit_position = BitmapType{1} << pos;
    return (Bitmap_ & bit_position) != 0;
  }
  ///@}

  /// \name Iterators
  ///@{
  /// Returns an iterator to the beginning of the container.
  iterator begin() { return data(); }
  const_iterator begin() const { return data(); }
  const_iterator cbegin() const { return begin(); }

  iterator end() { return begin() + size(); }
  const_iterator end() const { return begin() + size(); }
  const_iterator cend() const { return end(); }

  reverse_iterator rbegin() { return reverse_iterator{end()}; }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator{end()};
  }
  const_reverse_iterator crbegin() const { return rbegin(); }

  reverse_iterator rend() { return reverse_iterator{begin()}; }
  const_reverse_iterator rend() const {
    return const_reverse_iterator{begin()};
  }
  const_reverse_iterator crend() const { return rend(); }

  class Indices {
  public:
    class const_iterator {
    public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = std::size_t;
      using difference_type = std::ptrdiff_t;
      using pointer = value_type const *;
      using reference = value_type const &;

      const_iterator(BitmapType Bitmap) : Bitmap_{Bitmap}, Pos_{0} { next(); }

      bool operator==(const_iterator const &Rhs) const {
        return Bitmap_ == Rhs.Bitmap_;
      }
      bool operator!=(const_iterator const &Rhs) const {
        return !operator==(Rhs);
      }

      reference operator*() const { return Pos_; }
      pointer operator->() const { return Pos_; }

      const_iterator &operator++() {
        Bitmap_ >>= 1;
        ++Pos_;
        next();
        return *this;
      }

      const_iterator &operator++(int) {
        const_iterator prev = *this;
        next();
        return prev;
      }

    private:
      void next() {
        for (; Bitmap_ != 0 && (Bitmap_ & 1U) == 0U; ++Pos_, Bitmap_ >>= 1) {
        }
      }
      BitmapType Bitmap_;
      std::size_t Pos_;
    };
    explicit Indices(SparseArray const &Arr) : Bitmap_{Arr.Bitmap_} {}
    const_iterator begin() const { return {Bitmap_}; }
    const_iterator end() const { return {0}; }

  private:
    BitmapType const Bitmap_;
  };

  Indices getIndices() const { return Indices{*this}; }

  ///@}

  /// \name Element access
  ///@{

  ValueType *data() { return &Elems_[0]; }
  const ValueType *data() const { return &Elems_[0]; }

  reference operator[](size_type Pos) {
    SparseArray const *cthis = this;
    return const_cast<reference>(cthis->operator[](Pos));
  }
  const_reference operator[](size_type Pos) const {
    assert(Pos < max_size());
    auto const BitPosition = BitmapType{1} << Pos;
    assert((Bitmap_ & BitPosition) != 0);
    auto const Mask = BitPosition - 1U;
    auto const Index = popCount(Bitmap_ & Mask);
    return Elems_[Index];
  }

  reference at(size_type Pos) {
    SparseArray const *cthis = this;
    return const_cast<reference>(cthis->at(Pos));
  }
  const_reference at(size_type Pos) const {
    if (Pos < max_size()) {
      auto const BitPosition = BitmapType{1} << Pos;
      if ((Bitmap_ & BitPosition) != 0) {
        auto const Mask = BitPosition - 1U;
        auto const Index = popCount(Bitmap_ & Mask);
        return Elems_[Index];
      }
    }
    assert(!"SparseArray::at out_of_range");
  }

  ///@}

  void fill(ValueType const &Value) { std::fill_n(begin(), size(), Value); }

  constexpr std::size_t size_bytes() const {
    return SparseArray::size_bytes(this->size());
  }
  static constexpr std::size_t size_bytes(std::size_t NumEntries) {
    static_assert(sizeof(Elems_) / sizeof(ValueType) == 1U,
                  "Expected Elems_ to be an array of 1 ValueType");
    return sizeof(SparseArray) +
           (std::max(std::size_t{1}, NumEntries) - 1U) * sizeof(ValueType);
  }

  template <typename InputIterator>
  static std::size_t allocateBytes(std::size_t Count, InputIterator First,
                                   InputIterator Last) {
    // There will always be sufficient storage for at least 1 instance of
    // ValueType (this comes from the built-in array).
    static_assert(sizeof(Elems_) / sizeof(ValueType) == 1U,
                  "Expected Elems_ to be an array of 1 ValueType");
    auto const Elements = std::max(popCount(bitmap(First, Last)), 1U);
    return Count - sizeof(Elems_) + Elements * sizeof(ValueType);
  }

  static void *operator new(std::size_t /*Count*/, void *Ptr) { return Ptr; }
  static void operator delete(void * /*Ptr*/, void * /*Place*/) {}
  static void operator delete(void *P) { ::operator delete(P); }

  /// Constructs a sparse array whose available indices are defined by the
  /// iterator range from [FirstIndex,LastIndex) and the values assigned to
  /// those indices are given by [FirstValue, LastValue). If the number of
  /// elements in [FirstValue, LastValue) is less than the number of elements in
  /// [FirstIndex,LastIndex), the remaining values are default-constructed; if
  /// it is greater then the remaining values are ignored.
  template <typename IteratorIdx, typename IteratorV>
  SparseArray(IteratorIdx FirstIndex, IteratorIdx LastIndex,
              IteratorV FirstValue, IteratorV LastValue);
  /// Constructs a sparse array whose available indices are defined by the
  /// iterator range from [FirstIndex,LastIndex) and whose corresponding values
  /// are default constructed.
  template <typename IteratorIdx>
  SparseArray(IteratorIdx FirstIndex, IteratorIdx LastIndex);

private:
  /// \param Count  The number of bytes to allocate (to which the storage
  /// required for the array
  /// elements will be added).
  /// \param IndicesFirst  The beginning of the range of index values.
  /// \param IndicesLast  The end of the range of index values.
  /// \returns A pointer to the newly allocated memory block.
  template <typename InputIterator>
  static void *operator new(std::size_t Count, InputIterator IndicesFirst,
                            InputIterator IndicesLast) {
    return ::operator new(
        SparseArray::allocateBytes(Count, IndicesFirst, IndicesLast));
  }

  /// Placement delete member function to match the placement new member
  /// function which takes two input iterators.
  template <typename InputIterator>
  static void operator delete(void *P, InputIterator /*IndicesFirst*/,
                              InputIterator /*IndicesLast*/) {
    return ::operator delete(P);
  }

  BitmapType const Bitmap_;
  ValueType Elems_[1];

  /// Computes the bitmap value given a pair of iterators which will produce the
  /// sequence of indices to be present in a sparse array.
  template <typename InputIterator>
  static BitmapType bitmap(InputIterator First, InputIterator Last);
};

// (ctor)
// ~~~~~~
template <typename ValueType, typename BitmapType>
template <typename IteratorIdx, typename IteratorV>
SparseArray<ValueType, BitmapType>::SparseArray(IteratorIdx FirstIndex,
                                                IteratorIdx LastIndex,
                                                IteratorV FirstValue,
                                                IteratorV LastValue)
    : Bitmap_{bitmap(FirstIndex, LastIndex)} {

  // Deal with the first element. This is the odd-one-out becuase it's in the
  // declaration of the object and will have been default-constructed by the
  // compiler.
  if (FirstIndex != LastIndex) {
    if (FirstValue != LastValue) {
      Elems_[0] = *(FirstValue++);
    }
    ++FirstIndex;
  }

  auto Out = &Elems_[1];

  // Now construct any remaining elements into the uninitialized memory past the
  // end of the object using placement new.
  for (; FirstIndex != LastIndex && FirstValue != LastValue;
       ++FirstIndex, ++FirstValue, ++Out) {
    new (Out) ValueType(*FirstValue);
  }
  // Default-construct any remaining objects for which we don't have a value.
  for (; FirstIndex != LastIndex; ++FirstIndex, ++Out) {
    new (Out) ValueType();
  }
}

template <typename ValueType, typename BitmapType>
template <typename IteratorIdx>
SparseArray<ValueType, BitmapType>::SparseArray(IteratorIdx FirstIndex,
                                                IteratorIdx LastIndex)
    : Bitmap_{bitmap(FirstIndex, LastIndex)} {}

// (dtor)
// ~~~~~~
template <typename ValueType, typename BitmapType>
SparseArray<ValueType, BitmapType>::~SparseArray() {
  auto const Elements = size();
  if (Elements > 1) {
    for (auto It = &Elems_[1], end = &Elems_[0] + Elements; It != end; ++It) {
      It->~ValueType();
    }
  }
}

// make_unique
// ~~~~~~~~~~~
template <typename ValueType, typename BitmapType>
template <typename Iterator>
auto SparseArray<ValueType, BitmapType>::make_unique(Iterator Begin,
                                                     Iterator End)
    -> std::unique_ptr<SparseArray> {

  auto BeginFirst =
      details::makePairFieldIterator(Begin, details::firstAccessor<Iterator>);
  auto EndFirst =
      details::makePairFieldIterator(End, details::firstAccessor<Iterator>);
  auto BeginSecond =
      details::makePairFieldIterator(Begin, details::secondAccessor<Iterator>);
  auto EndSecond =
      details::makePairFieldIterator(End, details::secondAccessor<Iterator>);
  return std::unique_ptr<SparseArray<ValueType, BitmapType>>{
      new (BeginFirst, EndFirst)
          SparseArray<ValueType>(BeginFirst, EndFirst, BeginSecond, EndSecond)};
}

template <typename ValueType, typename BitmapType>
template <typename IteratorIdx, typename IteratorV>
auto SparseArray<ValueType, BitmapType>::make_unique(IteratorIdx FirstIndex,
                                                     IteratorIdx LastIndex,
                                                     IteratorV FirstValue,
                                                     IteratorV LastValue)
    -> std::unique_ptr<SparseArray> {

  return std::unique_ptr<SparseArray<ValueType>>{
      new (FirstIndex, LastIndex)
          SparseArray<ValueType>(FirstIndex, LastIndex, FirstValue, LastValue)};
}

template <typename ValueType, typename BitmapType>
auto SparseArray<ValueType, BitmapType>::make_unique(
    std::initializer_list<size_type> Indices,
    std::initializer_list<ValueType> Values) -> std::unique_ptr<SparseArray> {

  return make_unique(std::begin(Indices), std::end(Indices), std::begin(Values),
                     std::end(Values));
}

template <typename ValueType, typename BitmapType>
template <typename IteratorIdx>
auto SparseArray<ValueType, BitmapType>::make_unique(
    IteratorIdx FirstIndex, IteratorIdx LastIndex,
    std::initializer_list<ValueType> Values) -> std::unique_ptr<SparseArray> {

  return std::unique_ptr<SparseArray<ValueType>>{
      new (FirstIndex, LastIndex) SparseArray<ValueType>(
          FirstIndex, LastIndex, std::begin(Values), std::end(Values))};
}

// bitmap
// ~~~~~~
template <typename ValueType, typename BitmapType>
template <typename InputIterator>
BitmapType SparseArray<ValueType, BitmapType>::bitmap(InputIterator First,
                                                      InputIterator Last) {
  using IterValueType =
      typename std::iterator_traits<InputIterator>::value_type;
  auto Op = [](BitmapType Bm, IterValueType V) {
    auto Idx = static_cast<unsigned>(V);
    assert(Idx >= 0 && Idx < max_size());
    return Bm | (BitmapType{1} << Idx);
  };
  return std::accumulate(First, Last, BitmapType{0}, Op);
}

// operator==
// ~~~~~~~~~~
template <typename ValueType, typename BitmapType>
inline bool operator==(SparseArray<ValueType, BitmapType> const &Lhs,
                       SparseArray<ValueType, BitmapType> const &Rhs) {
  return Lhs.Bitmap_ == Rhs.Bitmap_ &&
         std::equal(std::begin(Lhs), std::end(Lhs), std::begin(Rhs));
}

// operator!=
// ~~~~~~~~~~
template <typename ValueType, typename BitmapType>
inline bool operator!=(SparseArray<ValueType, BitmapType> const &Lhs,
                       SparseArray<ValueType, BitmapType> const &Rhs) {
  return !(Lhs == Rhs);
}
} // end namespace repo
} // end namespace pstore

namespace std {

template <std::size_t Ip, typename ValueType, typename BitmapType>
inline constexpr ValueType &
get(pstore::repo::SparseArray<ValueType, BitmapType> &Arr) noexcept {
  static_assert(Ip < pstore::repo::SparseArray<ValueType, BitmapType>::max_size(),
                "Index out of bounds in std::get<> (SparseArray)");
  return Arr[Ip];
}

template <std::size_t Ip, typename ValueType, typename BitmapType>
inline constexpr ValueType const &
get(pstore::repo::SparseArray<ValueType, BitmapType> const &Arr) noexcept {
  static_assert(Ip < pstore::repo::SparseArray<ValueType, BitmapType>::max_size(),
                "Index out of bounds in std::get<> (const SparseArray)");
  return Arr[Ip];
}

} // end namespace std

#endif // LLVM_MCREPOFRAGMENT_SPARSE_ARRAY_H
// eof: include/pstore_mcrepo/SparseArray.h
