//===- include/pstore/support/gsl.hpp ---------------------*- mode: C++ -*-===//
//*            _  *
//*   __ _ ___| | *
//*  / _` / __| | *
//* | (_| \__ \ | *
//*  \__, |___/_| *
//*  |___/        *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_SUPPORT_GSL_HPP
#define PSTORE_SUPPORT_GSL_HPP

#include <array>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "pstore/support/assert.hpp"

namespace pstore {
    namespace gsl {
        //
        // czstring and wzstring
        //
        // These are "tag" typedef's for C-style strings (i.e. null-terminated character arrays)
        // that allow static analysis to help find bugs.
        //
        // There are no additional features/semantics that we can find a way to add inside the
        // type system for these types that will not either incur significant runtime costs or
        // (sometimes needlessly) break existing programs when introduced.
        //

        template <typename CharT>
        using basic_zstring = CharT *;

        using zstring = basic_zstring<char>;
        using wzstring = basic_zstring<wchar_t>;
        using czstring = basic_zstring<char const>;
        using cwzstring = basic_zstring<wchar_t const>;



        // [views.constants], constants
        constexpr const std::ptrdiff_t dynamic_extent = -1;

        template <typename ElementType, std::ptrdiff_t Extent = dynamic_extent>
        class span;

        // implementation details
        namespace details {
            template <typename Span, bool IsConst>
            class span_iterator {
                using element_type = typename Span::element_type;

            public:
                using iterator_category = std::random_access_iterator_tag;
                using value_type = typename std::remove_const<element_type>::type;
                using difference_type = typename Span::index_type;

                using reference =
                    typename std::conditional<IsConst, element_type const, element_type>::type &;
                using pointer = typename std::add_pointer<reference>::type;

                friend class span_iterator<Span, true>;

                constexpr span_iterator () noexcept = default;
                constexpr span_iterator (Span const * span,
                                         typename Span::index_type index) noexcept
                        : span_ (span)
                        , index_ (index) {
                    PSTORE_ASSERT (span == nullptr || (index_ >= 0 && index <= span_->length ()));
                }
                // Note that we allow non-const iterators to be assigned and constructed from
                // const iterators, but not the other way round. We want to allow implicit
                // conversions between span types.
                // NOLINTNEXTLINE(hicpp-explicit-conversions)
                constexpr span_iterator (span_iterator<Span, false> const & other) noexcept
                        : span_iterator (other.span_, other.index_) {}

                template <bool OtherIsConst = IsConst,
                          typename = typename std::enable_if<OtherIsConst>::type>
                // NOLINTNEXTLINE(hicpp-explicit-conversions)
                constexpr span_iterator (span_iterator<Span, true> const & other) noexcept
                        : span_iterator (other.span_, other.index_) {}

                span_iterator & operator= (span_iterator<Span, false> const & rhs) noexcept {
                    span_ = rhs.span_;
                    index_ = rhs.index_;
                    return *this;
                }
                template <bool OtherIsConst = IsConst,
                          typename = typename std::enable_if<OtherIsConst>::type>
                span_iterator & operator= (span_iterator<Span, true> const & rhs) noexcept {
                    span_ = rhs.span_;
                    index_ = rhs.index_;
                    return *this;
                }

                reference operator* () const {
                    PSTORE_ASSERT (span_);
                    return (*span_)[index_];
                }

                pointer operator-> () const {
                    PSTORE_ASSERT (span_);
                    return &((*span_)[index_]);
                }

                span_iterator & operator++ () noexcept {
                    PSTORE_ASSERT (span_ && index_ >= 0 && index_ < span_->length ());
                    ++index_;
                    return *this;
                }

                span_iterator operator++ (int) noexcept {
                    auto ret = *this;
                    ++(*this);
                    return ret;
                }

                span_iterator & operator-- () noexcept {
                    PSTORE_ASSERT (span_ && index_ > 0 && index_ <= span_->length ());
                    --index_;
                    return *this;
                }

                span_iterator operator-- (int) const noexcept {
                    auto ret = *this;
                    --(*this);
                    return ret;
                }

                span_iterator operator+ (difference_type n) const noexcept {
                    auto ret = *this;
                    return ret += n;
                }

                span_iterator & operator+= (difference_type n) {
                    PSTORE_ASSERT (span_ && (index_ + n) >= 0 && (index_ + n) <= span_->length ());
                    index_ += n;
                    return *this;
                }

                span_iterator & operator-= (difference_type n) noexcept { return *this += -n; }

                span_iterator operator- (difference_type n) const noexcept {
                    auto ret = *this;
                    return ret -= n;
                }
                difference_type operator- (span_iterator const & rhs) const {
                    PSTORE_ASSERT (span_ == rhs.span_);
                    return index_ - rhs.index_;
                }

                constexpr reference operator[] (difference_type n) const noexcept {
                    return *(*this + n);
                }

                constexpr friend bool operator== (span_iterator const & lhs,
                                                  span_iterator const & rhs) noexcept {
                    return lhs.span_ == rhs.span_ && lhs.index_ == rhs.index_;
                }

                constexpr friend bool operator!= (span_iterator const & lhs,
                                                  span_iterator const & rhs) noexcept {
                    return !(lhs == rhs);
                }

                friend bool operator< (span_iterator const & lhs, span_iterator const & rhs) {
                    PSTORE_ASSERT (lhs.span_ == rhs.span_);
                    return lhs.index_ < rhs.index_;
                }

                friend bool operator<= (span_iterator const & lhs,
                                        span_iterator const & rhs) noexcept {
                    return !(rhs < lhs);
                }

                friend bool operator> (span_iterator const & lhs,
                                       span_iterator const & rhs) noexcept {
                    return rhs < lhs;
                }

                friend bool operator>= (span_iterator const & lhs,
                                        span_iterator const & rhs) noexcept {
                    return !(rhs > lhs);
                }

                void swap (span_iterator & rhs) noexcept {
                    std::swap (index_, rhs.index_);
                    std::swap (span_, rhs.span_);
                }

            private:
                Span const * span_ = nullptr;
                std::ptrdiff_t index_ = 0;
            };

            template <typename Span, bool IsConst>
            constexpr span_iterator<Span, IsConst>
            operator+ (typename span_iterator<Span, IsConst>::difference_type n,
                       span_iterator<Span, IsConst> const & rhs) noexcept {
                return rhs + n;
            }

            template <typename Span, bool IsConst>
            constexpr span_iterator<Span, IsConst>
            operator- (typename span_iterator<Span, IsConst>::difference_type n,
                       span_iterator<Span, IsConst> const & rhs) noexcept {
                return rhs - n;
            }

            template <std::ptrdiff_t Extent>
            class extent_type {
            public:
                using index_type = std::ptrdiff_t;

                static_assert (Extent >= 0, "A fixed-size span must be >= 0 in size.");

                constexpr extent_type () noexcept = default;

                template <index_type Other>
                constexpr explicit extent_type (extent_type<Other> ext) {
                    static_assert (
                        Other == Extent || Other == dynamic_extent,
                        "Mismatch between fixed-size extent and size of initializing data.");
                    (void) ext;
                    PSTORE_ASSERT (ext.size () == Extent);
                }

                constexpr explicit extent_type (index_type const size) noexcept {
                    (void) size;
                    PSTORE_ASSERT (size == Extent);
                }

                constexpr index_type size () const noexcept { return Extent; }
            };

            template <>
            class extent_type<dynamic_extent> {
            public:
                using index_type = std::ptrdiff_t;

                template <index_type Other>
                constexpr explicit extent_type (extent_type<Other> const ext) noexcept
                        : size_ (ext.size ()) {}
                constexpr explicit extent_type (index_type const size) noexcept
                        : size_ (size) {
                    PSTORE_ASSERT (size >= 0);
                }
                constexpr index_type size () const noexcept { return size_; }

            private:
                index_type size_;
            };
        } // namespace details


        // [span], class template span
        template <typename ElementType, std::ptrdiff_t Extent>
        class span {
        public:
            // constants and types
            using element_type = ElementType;
            using index_type = std::ptrdiff_t;
            using pointer = element_type *;
            using reference = element_type &;

            using iterator = details::span_iterator<span<ElementType, Extent>, false>;
            using const_iterator = details::span_iterator<span<ElementType, Extent>, true>;
            using reverse_iterator = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            constexpr static index_type const extent = Extent;

            // [span.cons], span constructors, copy, assignment, and destructor
            constexpr span () noexcept
                    : storage_ (nullptr, details::extent_type<0> ()) {}
            constexpr explicit span (std::nullptr_t const) noexcept
                    : span () {}
            constexpr span (pointer ptr, index_type count)
                    : storage_ (ptr, count) {}

            constexpr span (pointer first, pointer last)
                    : storage_ (first, std::distance (first, last)) {}


            template <size_t N>
            constexpr explicit span (element_type (&arr)[N]) noexcept
                    : storage_ (&arr[0], details::extent_type<N> ()) {}

            template <size_t N, class ArrayElementType = element_type>
            constexpr explicit span (std::array<ArrayElementType, N> & arr) noexcept
                    : storage_ (&arr[0], details::extent_type<N> ()) {}

            template <size_t N, class ArrayElementType = element_type>
            constexpr explicit span (std::array<ArrayElementType, N> const & arr) noexcept
                    : storage_ (&arr[0], details::extent_type<N> ()) {}


            template <typename MemberType>
            constexpr explicit span (std::vector<MemberType> & cont)
                    : span (cont.data (), static_cast<index_type> (cont.size ())) {}

            template <typename MemberType>
            constexpr explicit span (std::vector<MemberType> const & cont)
                    : span (cont.data (), static_cast<index_type> (cont.size ())) {}


            template <typename ArrayElementType = std::add_pointer<element_type>>
            constexpr span (std::unique_ptr<ArrayElementType> const & ptr, index_type count)
                    : storage_ (ptr.get (), count) {}
            constexpr explicit span (std::unique_ptr<element_type> const & ptr)
                    : storage_ (ptr.get (), ptr.get () ? 1 : 0) {}
            constexpr explicit span (std::shared_ptr<element_type> const & ptr)
                    : storage_ (ptr.get (), ptr.get () ? 1 : 0) {}


            template <typename OtherElementType, std::ptrdiff_t OtherExtent>
            // NOLINTNEXTLINE(hicpp-explicit-conversions)
            constexpr span (span<OtherElementType, OtherExtent> const & other)
                    : storage_ (other.data (), details::extent_type<OtherExtent> (other.size ())) {}


            constexpr span (span const & other) noexcept = default;
            constexpr span (span && other) noexcept = default;

            ~span () noexcept = default;
            span & operator= (span const & other) noexcept = default;
            span & operator= (span && other) noexcept = default;

            // [span.sub], span subviews
            template <std::ptrdiff_t Count>
            span<element_type, Count> first () const {
                PSTORE_ASSERT (Count >= 0 && Count <= size ());
                return {data (), Count};
            }

            span<element_type, dynamic_extent> first (index_type count) const {
                PSTORE_ASSERT (count >= 0 && count <= size ());
                return {data (), count};
            }

            template <std::ptrdiff_t Count>
            span<element_type, Count> last () const {
                PSTORE_ASSERT (Count >= 0 && Count <= size ());
                return {data () + (size () - Count), Count};
            }

            span<element_type, dynamic_extent> last (index_type count) const {
                PSTORE_ASSERT (count >= 0 && count <= size ());
                return {data () + (size () - count), count};
            }

            template <std::ptrdiff_t Offset, std::ptrdiff_t Count = dynamic_extent>
            span<element_type, Count> subspan () const {
                PSTORE_ASSERT (
                    (Offset == 0 || (Offset > 0 && Offset <= size ())) &&
                    (Count == dynamic_extent || (Count >= 0 && Offset + Count <= size ())));
                return {data () + Offset, Count == dynamic_extent ? size () - Offset : Count};
            }

            span<element_type, dynamic_extent>
            subspan (index_type const offset, index_type const count = dynamic_extent) const
                noexcept {
                PSTORE_ASSERT (
                    (offset == 0 || (offset > 0 && offset <= size ())) &&
                    (count == dynamic_extent || (count >= 0 && offset + count <= size ())));
                return {data () + offset, count == dynamic_extent ? size () - offset : count};
            }

            // [span.obs], span observers
            constexpr index_type length () const noexcept { return size (); }
            constexpr index_type size () const noexcept { return storage_.size (); }
            constexpr index_type length_bytes () const noexcept { return size_bytes (); }
            constexpr index_type size_bytes () const noexcept {
                return size () * static_cast<index_type> (sizeof (element_type));
            }
            constexpr bool empty () const noexcept { return size () == 0; }

            // [span.elem], span element access
            reference operator[] (index_type const idx) const {
                PSTORE_ASSERT (idx >= 0 && idx < storage_.size ());
                return data ()[idx];
            }

            constexpr reference at (index_type const idx) const { return this->operator[] (idx); }
            constexpr reference operator() (index_type const idx) const {
                return this->operator[] (idx);
            }
            constexpr pointer data () const noexcept { return storage_.data (); }

            // [span.iter], span iterator support
            iterator begin () const noexcept { return {this, 0}; }
            iterator end () const noexcept { return {this, length ()}; }

            const_iterator cbegin () const noexcept { return {this, 0}; }
            const_iterator cend () const noexcept { return {this, length ()}; }

            reverse_iterator rbegin () const noexcept { return reverse_iterator{end ()}; }
            reverse_iterator rend () const noexcept { return reverse_iterator{begin ()}; }

            const_reverse_iterator crbegin () const noexcept {
                return const_reverse_iterator{cend ()};
            }
            const_reverse_iterator crend () const noexcept {
                return const_reverse_iterator{cbegin ()};
            }

        private:
            // this implementation detail class lets us take advantage of the
            // empty base class optimization to pay for only storage of a single
            // pointer in the case of fixed-size spans
            template <typename ExtentType>
            class storage_type : public ExtentType {
            public:
                template <typename OtherExtentType>
                constexpr storage_type (pointer data, OtherExtentType ext)
                        : ExtentType (ext)
                        , data_ (data) {}

                constexpr pointer data () const noexcept { return data_; }

            private:
                pointer data_;
            };

            storage_type<details::extent_type<Extent>> storage_;
        };

        template <typename ElementType, std::ptrdiff_t Extent>
        constexpr
            typename span<ElementType, Extent>::index_type const span<ElementType, Extent>::extent;


        // [span.comparison], span comparison operators
        template <typename ElementType, std::ptrdiff_t FirstExtent, std::ptrdiff_t SecondExtent>
        constexpr bool operator== (span<ElementType, FirstExtent> const & lhs,
                                   span<ElementType, SecondExtent> const & rhs) {

            return lhs.size () == rhs.size () &&
                   std::equal (lhs.begin (), lhs.end (), rhs.begin ());
        }

        template <typename ElementType, std::ptrdiff_t Extent>
        constexpr bool operator!= (span<ElementType, Extent> const & lhs,
                                   span<ElementType, Extent> const & rhs) {
            return !(lhs == rhs);
        }

        template <typename ElementType, std::ptrdiff_t Extent>
        constexpr bool operator< (span<ElementType, Extent> const & lhs,
                                  span<ElementType, Extent> const & rhs) {
            return std::lexicographical_compare (lhs.begin (), lhs.end (), rhs.begin (),
                                                 rhs.end ());
        }

        template <typename ElementType, std::ptrdiff_t Extent>
        constexpr bool operator<= (span<ElementType, Extent> const & lhs,
                                   span<ElementType, Extent> const & rhs) {
            return !(lhs > rhs);
        }

        template <typename ElementType, std::ptrdiff_t Extent>
        constexpr bool operator> (span<ElementType, Extent> const & lhs,
                                  span<ElementType, Extent> const & rhs) {
            return rhs < lhs;
        }

        template <typename ElementType, std::ptrdiff_t Extent>
        constexpr bool operator>= (span<ElementType, Extent> const & lhs,
                                   span<ElementType, Extent> const & rhs) {
            return !(lhs < rhs);
        }


        namespace details {
            // if we only supported compilers with good constexpr support then
            // this pair of classes could collapse down to a constexpr function

            // we should use a narrow_cast<> to go to size_t, but older compilers may not see it as
            // constexpr and so will fail compilation of the template
            template <typename ElementType, std::ptrdiff_t Extent>
            struct calculate_byte_size
                    : std::integral_constant<std::ptrdiff_t,
                                             static_cast<std::ptrdiff_t> (
                                                 sizeof (ElementType) *
                                                 static_cast<std::size_t> (Extent))> {};

            template <typename ElementType>
            struct calculate_byte_size<ElementType, dynamic_extent>
                    : std::integral_constant<std::ptrdiff_t, dynamic_extent> {};
        } // namespace details

        // [span.objectrep], views of object representation
        template <typename ElementType, std::ptrdiff_t Extent>
        span<std::uint8_t const, details::calculate_byte_size<ElementType, Extent>::value>
        as_bytes (span<ElementType, Extent> s) noexcept {
            return {reinterpret_cast<std::uint8_t const *> (s.data ()), s.size_bytes ()};
        }

        template <typename ElementType, std::ptrdiff_t Extent,
                  class = typename std::enable_if<!std::is_const<ElementType>::value>::type>
        span<std::uint8_t, details::calculate_byte_size<ElementType, Extent>::value>
        as_writeable_bytes (span<ElementType, Extent> s) noexcept {
            return {reinterpret_cast<std::uint8_t *> (s.data ()), s.size_bytes ()};
        }


        //
        // make_span() - Utility functions for creating spans
        //
        template <typename ElementType>
        span<ElementType> make_span (ElementType * ptr,
                                     typename span<ElementType>::index_type count) {
            return span<ElementType> (ptr, count);
        }

        template <typename ElementType>
        span<ElementType> make_span (ElementType * first, ElementType * last) {
            return span<ElementType> (first, last);
        }

        template <typename ElementType, std::size_t N>
        span<ElementType, N> make_span (ElementType (&arr)[N]) {
            return span<ElementType, N>{arr};
        }


        template <typename ValueType, std::size_t N>
        span<ValueType, N> make_span (std::array<ValueType, N> & cont) {
            return span<ValueType, N>{cont};
        }
        template <typename ValueType, std::size_t N>
        span<ValueType const, N> make_span (std::array<ValueType, N> const & cont) {
            return span<ValueType const, N>{cont};
        }



        template <typename Container>
        span<typename Container::value_type> make_span (Container & c) {
            return {c.data (), static_cast<std::ptrdiff_t> (c.size ())};
        }
        template <typename Container>
        span<typename Container::value_type const> make_span (Container const & c) {
            return {c.data (), static_cast<std::ptrdiff_t> (c.size ())};
        }



        template <typename SmartPtr>
        span<typename SmartPtr::element_type> make_span (SmartPtr & cont, std::ptrdiff_t count) {
            return {cont, count};
        }

        template <typename SmartPtr>
        span<typename SmartPtr::element_type> make_span (SmartPtr & cont) {
            return span<typename SmartPtr::element_type>{cont};
        }


        //
        // not_null
        //
        // Restricts a pointer or smart pointer to only hold non-null values.
        //
        // Has zero size overhead over T.
        //
        // If T is a pointer (i.e. T == U*) then
        // - allow construction from U* or U&
        // - disallow construction from nullptr_t
        // - disallow default construction
        // - ensure construction from U* fails with nullptr
        // - allow implicit conversion to U*
        //
        template <typename T>
        class not_null {
            static_assert (std::is_assignable<T &, std::nullptr_t>::value,
                           "T cannot be assigned nullptr.");

        public:
            // NOLINTNEXTLINE(hicpp-explicit-conversions)
            constexpr not_null (T const t) noexcept
                    : ptr_{t} {
                ensure_invariant ();
            }

            // prevents compilation when someone attempts to assign a nullptr
            not_null (std::nullptr_t) noexcept = delete;
            not_null (int) noexcept = delete;
            not_null (not_null const &) noexcept = default;
            not_null (not_null &&) noexcept = default;

            ~not_null () noexcept = default;

            not_null & operator= (not_null const &) noexcept = default;
            not_null & operator= (not_null &&) noexcept = default;
            not_null & operator= (T const & t) noexcept {
                ptr_ = t;
                ensure_invariant ();
                return *this;
            }

            // prevents compilation when someone attempts to assign a nullptr
            not_null<T> & operator= (std::nullptr_t) = delete;
            not_null<T> & operator= (int) = delete;

            constexpr T get () const noexcept { return ptr_; }

            // NOLINTNEXTLINE(hicpp-explicit-conversions)
            constexpr operator T () const noexcept { return get (); }
            constexpr T operator-> () const noexcept { return get (); }

            bool operator== (T const & rhs) const noexcept { return ptr_ == rhs; }
            bool operator!= (T const & rhs) const noexcept { return !(*this == rhs); }

            // unwanted operators...pointers only point to single objects!
            // TODO ensure all arithmetic ops on this type are unavailable
            not_null<T> & operator++ () = delete;
            not_null<T> & operator-- () = delete;
            not_null<T> operator++ (int) = delete;
            not_null<T> operator-- (int) = delete;
            not_null<T> & operator+ (size_t) = delete;
            not_null<T> & operator+= (size_t) = delete;
            not_null<T> & operator- (size_t) = delete;
            not_null<T> & operator-= (size_t) = delete;

        private:
            T ptr_;

            // We assume that the compiler can hoist/prove away most of the checks inlined from this
            // function. If not, we could make them optional via conditional compilation.
            constexpr void ensure_invariant () const noexcept { PSTORE_ASSERT (ptr_ != nullptr); }
        };


        //
        // at() - Bounds-checked way of accessing static arrays, std::array, std::vector
        //
        template <typename T, size_t N>
        constexpr T & at (T (&arr)[N], std::ptrdiff_t const index) {
            PSTORE_ASSERT (index >= 0 && index < static_cast<std::ptrdiff_t> (N));
            return arr[static_cast<size_t> (index)];
        }

        template <typename T, size_t N>
        constexpr T & at (std::array<T, N> & arr, std::ptrdiff_t const index) {
            PSTORE_ASSERT (index >= 0 && index < static_cast<std::ptrdiff_t> (N));
            return arr[static_cast<size_t> (index)];
        }

        template <typename Cont>
        constexpr typename Cont::value_type & at (Cont & cont, std::ptrdiff_t const index) {
            PSTORE_ASSERT (index >= 0 && index < static_cast<std::ptrdiff_t> (cont.size ()));
            return cont[static_cast<typename Cont::size_type> (index)];
        }

        template <typename T>
        constexpr T const & at (std::initializer_list<T> cont, std::ptrdiff_t const index) {
            PSTORE_ASSERT (index >= 0 && index < static_cast<std::ptrdiff_t> (cont.size ()));
            return *(cont.begin () + index);
        }

    } // namespace gsl
} // end namespace pstore

#endif // PSTORE_SUPPORT_GSL_HPP
