#ifndef PSTORE_EXCHANGE_EXPORT_EMIT_HPP
#define PSTORE_EXCHANGE_EXPORT_EMIT_HPP

#include <algorithm>
#include <string>

#include "pstore/adt/sstring_view.hpp"
#include "pstore/support/gsl.hpp"


namespace pstore {
    namespace exchange {

        namespace details {

            template <typename OSStream, typename T>
            void write_span (OSStream & os, gsl::span<T> const & sp) {
                os.write (sp.data (), sp.length ());
            }

        } // end namespace details

        template <typename OStream, typename Iterator>
        void emit_string (OStream & os, Iterator first, Iterator last) {
            os << '"';
            auto pos = first;
            while ((pos = std::find_if (first, last,
                                        [] (char c) { return c == '"' || c == '\\'; })) != last) {
                details::write_span (os, gsl::make_span (&*first, pos - first));
                os << '\\' << *pos;
                first = pos + 1;
            }
            if (first != last) {
                details::write_span (os, gsl::make_span (&*first, last - first));
            }
            os << '"';
        }

        template <typename OStream>
        void emit_string (OStream & os, raw_sstring_view const & view) {
            emit_string (os, std::begin (view), std::end (view));
        }

        template <typename OStream>
        void emit_string (OStream & os, std::string const & str) {
            emit_string (os, std::begin (str), std::end (str));
        }

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_EMIT_HPP
