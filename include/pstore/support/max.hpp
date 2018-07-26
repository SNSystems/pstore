/// \brief A template implementation of std::max() with helper types for determining the maximum
/// size and alignment of a collection of types.
///
/// In C++14, std::max() becomes a constexpr function which enables its use at compile time, and
/// more particularly allows for its use in template arguments. This library uses it as an argument
/// to std::aligned_storage<> so that we can produce a block of storage into which an instance of a
/// number of types could be constructed.
#ifndef PSTORE_SUPPORT_MAX_HPP
#define PSTORE_SUPPORT_MAX_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace pstore {

    // max
    // ~~~
    /// In C++14 std::max() is a constexpr function so we could use it in these templates,
    /// but our code needs to be compatible with C++11.
    template <typename T, T Value1, T Value2>
    struct max : std::integral_constant<T, (Value1 < Value2) ? Value2 : Value1> {};

    // max_sizeof
    // ~~~~~~~~~~
    template <typename... T>
    struct max_sizeof;
    template <>
    struct max_sizeof<> : std::integral_constant<std::size_t, 1U> {};
    template <typename Head, typename... Tail>
    struct max_sizeof<Head, Tail...>
            : std::integral_constant<
                  std::size_t, max<std::size_t, sizeof (Head), max_sizeof<Tail...>::value>::value> {
    };

    // max_alignment
    // ~~~~~~~~~~~~~
    template <typename... Types>
    struct max_alignment;
    template <>
    struct max_alignment<> : std::integral_constant<std::size_t, 1U> {};
    template <typename Head, typename... Tail>
    struct max_alignment<Head, Tail...>
            : std::integral_constant<std::size_t, max<std::size_t, alignof (Head),
                                                      max_alignment<Tail...>::value>::value> {};

    // characteristics
    // ~~~~~~~~~~~~~~~
    /// Given a list of types, find the size of the largest and the alignment of the most aligned.
    template <typename... T>
    struct characteristics {
        static constexpr std::size_t size = max_sizeof<T...>::value;
        static constexpr std::size_t align = max_alignment<T...>::value;
    };

} // end namespace pstore

static_assert (pstore::max<int, 1, 2>::value == 2, "max(1,2)!=2");
static_assert (pstore::max<int, 2, 1>::value == 2, "max(2,1)!=2");
static_assert (pstore::max_sizeof<std::uint_least8_t>::value == sizeof (std::uint_least8_t),
               "max(sizeof(8))!=sizeof(8)");
static_assert (pstore::max_sizeof<std::uint_least8_t, std::uint_least16_t>::value >=
                   sizeof (std::uint_least16_t),
               "max(sizeof(8),sizeof(16)!=4");

#endif // PSTORE_SUPPORT_MAX_HPP
