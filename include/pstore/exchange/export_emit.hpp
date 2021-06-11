//===- include/pstore/exchange/export_emit.hpp ------------*- mode: C++ -*-===//
//*                             _                    _ _    *
//*   _____  ___ __   ___  _ __| |_    ___ _ __ ___ (_) |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __|  / _ \ '_ ` _ \| | __| *
//* |  __/>  <| |_) | (_) | |  | |_  |  __/ | | | | | | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__|  \___|_| |_| |_|_|\__| *
//*           |_|                                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file export_emit.hpp
/// \brief Types and functions used in the writing of JSON data such as indents, strings, arrays,
/// and so on.

#ifndef PSTORE_EXCHANGE_EXPORT_EMIT_HPP
#define PSTORE_EXCHANGE_EXPORT_EMIT_HPP

#include "pstore/core/indirect_string.hpp"
#include "pstore/exchange/export_ostream.hpp"

namespace pstore {

    class database;

    namespace exchange {
        namespace export_ns {

            class indent {
            public:
                constexpr indent () noexcept = default;
                constexpr indent next () const noexcept { return indent{distance_ + 1U}; }
                constexpr unsigned distance () const noexcept { return distance_; }
                std::string str () const {
                    return std::string (static_cast<std::string::size_type> (distance () * 2U),
                                        ' ');
                }

            private:
                explicit constexpr indent (unsigned const distance) noexcept
                        : distance_{distance} {}
                unsigned distance_ = 0U;
            };

            ostream_base & operator<< (ostream_base & os, indent const & i);

            namespace details {

                template <typename ElementType, std::ptrdiff_t Extent>
                void write_span (ostream_base & os, gsl::span<ElementType, Extent> const & sp) {
                    os.write (sp.data (), sp.length ());
                }

            } // end namespace details


            /// \brief An OutputIterator intended for use with the diff() API.
            ///
            /// When the diff() function finds an index entry that it wants to report as new, it
            /// does so by writing to an OutputIterator. This implementation forwards the address of
            /// the new object to a function for processing.
            ///
            /// \tparam OutputFunction A function with signature equivalent to 'void(address').
            template <typename OutputFunction>
            class diff_out {
            public:
                using iterator_category = std::output_iterator_tag;
                using value_type = void;
                using difference_type = void;
                using pointer = void;
                using reference = void;

                explicit diff_out (OutputFunction const * const fn) noexcept
                        : fn_{fn} {}

                diff_out & operator= (pstore::address const addr) {
                    (*fn_) (addr);
                    return *this;
                }

                diff_out & operator* () noexcept { return *this; }
                diff_out & operator++ () noexcept { return *this; }
                diff_out operator++ (int) noexcept { return *this; }

            private:
                OutputFunction const * fn_;
            };

            template <typename OutputFunction>
            diff_out<OutputFunction> make_diff_out (OutputFunction const * const fn) {
                return diff_out<OutputFunction>{fn};
            }

            void emit_digest (ostream_base & os, uint128 const d);

            template <typename Iterator>
            void emit_string (ostream_base & os, Iterator first, Iterator last) {
                os << '"';
                auto pos = first;
                while ((pos = std::find_if (first, last, [] (char const c) {
                            return c == '"' || c == '\\';
                        })) != last) {
                    details::write_span (os, gsl::make_span (&*first, pos - first));
                    os << '\\' << *pos;
                    first = pos + 1;
                }
                if (first != last) {
                    details::write_span (os, gsl::make_span (&*first, last - first));
                }
                os << '"';
            }

            void emit_string (ostream_base & os, raw_sstring_view const & view);

            /// If \p comments is true, emits a comment containing the body of the string at address
            /// \p addr.
            ///
            /// \param os  The output stream to which the comment will be emitted.
            /// \param db  The database containing the string to be emitted.
            /// \param addr  The address of the indirect-string to be emitted.
            /// \param comments  A value indicating whether comment emission is enabled.
            /// \returns A reference to \p os.
            ostream_base & show_string (ostream_base & os, pstore::database const & db,
                                        pstore::typed_address<pstore::indirect_string> const addr,
                                        bool const comments);

            /// Writes an array of values given by the range \p first to \p last to the output
            /// stream \p os. The output follows the JSON syntax of "[ a, b ]" except that each
            /// object
            ///  (a, b) is written on a new line.
            ///
            /// \tparam OStream  An output stream type to which values can be written using the '<<'
            ///     operator.
            /// \tparam InputIt  An input iterator.
            /// \tparam Function  A callable whose signature should be equivalent to:
            ///     void fun(OStream &, indent, std::iterator_traits<InputIt>::value_type const &a);
            /// \param os  An output stream to which values are written using the '<<' operator.
            /// \param ind  The indentation to be applied to each member of the array.
            /// \param first  The start of the range denoting array elements to be emitted.
            /// \param last  The end of the range denoting array elements to be emitted.
            /// \param fn  A function which is called to emit the contents of each object in the
            ///    iterator range denoted by [first, last).
            template <typename OStream, typename InputIt, typename Function>
            OStream & emit_array (OStream & os, indent const ind, InputIt first, InputIt last,
                                  Function fn) {
                auto const * sep = "";
                auto const * tail_sep = "";
                auto tail_sep_indent = indent{};
                os << "[";
                std::for_each (first, last, [&] (auto const & element) {
                    os << sep << '\n';
                    fn (os, ind.next (), element);
                    sep = ",";
                    tail_sep = "\n";
                    tail_sep_indent = ind;
                });
                os << tail_sep << tail_sep_indent << "]";
                return os;
            }

            /// Writes an array of value given by the range \p first to \p last to the output
            /// stream \p os. The output follows the JSON syntax of "[ a, b ]" except that each
            /// object is written on a new line. The function \p fn should return an indirect-string
            /// address the value of which, if enabled by the user, will also be written to the
            /// output as a comment.
            ///
            /// \tparam OStream  An output stream type to which values can be written using the '<<'
            ///     operator.
            /// \tparam InputIt  An input iterator.
            /// \tparam Function  A callable whose signature should be equivalent to:
            ///
            ///     typed_address<indirect_string>
            ///     (OStream &, std::iterator_traits<InputIt>::value_type const &a)
            ///
            /// \param os  An output stream to which values are written using the '<<' operator.
            /// \param ind  The indentation to be applied to each member of the array.
            /// \param first  The start of the range denoting array elements to be emitted.
            /// \param last  The end of the range denoting array elements to be emitted.
            /// \param comments  Emit comments with a string for each array element?
            /// \param fn  A function which is called to emit the contents of each object in the
            ///    iterator range denoted by [first, last).
            template <typename OStream, typename InputIt, typename Function>
            OStream & emit_array_with_name (OStream & os, indent const ind, database const & db,
                                            InputIt const first, InputIt const last,
                                            bool const comments, Function const fn) {
                auto const * sep = "\n";
                os << "[";
                maybe<typed_address<indirect_string>> prev_name;
                indent const ind1 = ind.next ();

                std::for_each (first, last, [&] (auto const & element) {
                    os << sep;
                    if (prev_name) {
                        show_string (os, db, *prev_name, comments);
                        os << '\n';
                    }
                    os << ind1;
                    prev_name = fn (os, element);
                    sep = ",";
                });
                if (prev_name) {
                    show_string (os, db, *prev_name, comments);
                    os << '\n' << ind;
                }
                os << ']';
                return os;
            }

            /// Writes a object to the output stream \p os. The output consists of a pair of braces
            /// with appropriate whitespace. The function \p fn is called to write the properties
            /// and values of the object.
            ///
            /// \tparam OStream  An output stream type to which values can be written using the '<<'
            ///     operator.
            /// \tparam Object  The type of the object to be written.
            /// \tparam Function  A callable whose signature should be equivalent to:
            ///     void fun(OStream &, indent, Object);
            /// \param os  An output stream to which values are written using the '<<' operator.
            /// \param ind  The indentation to be applied to each member of the object.
            /// \param object  The object instance to be written.
            /// \param fn  A function which is called to emit the properties and values of the
            /// object.
            template <typename OStream, typename Object, typename Function>
            OStream & emit_object (OStream & os, indent const ind, Object const & object,
                                   Function fn) {
                os << "{\n";
                fn (os, ind.next (), object);
                os << ind << '}';
                return os;
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_EMIT_HPP
