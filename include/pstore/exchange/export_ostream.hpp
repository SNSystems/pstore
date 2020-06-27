#ifndef PSTORE_EXCHANGE_EXPORT_OSTREAM_HPP
#define PSTORE_EXCHANGE_EXPORT_OSTREAM_HPP

#include <cstdio>
#include <string>

#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace exchange {

        class crude_ostream {
        public:
            explicit crude_ostream (FILE * const os)
                    : os_ (os) {}
            crude_ostream (crude_ostream const &) = delete;
            crude_ostream & operator= (crude_ostream const &) = delete;

            crude_ostream & write (char c);
            crude_ostream & write (std::uint16_t v);
            crude_ostream & write (std::uint32_t v);
            crude_ostream & write (std::uint64_t v);
            crude_ostream & write (gsl::czstring str);
            crude_ostream & write (std::string const & str);

            template <typename T, typename = typename std::enable_if<true>::type>
            crude_ostream & write (T const * s, std::size_t length) {
                std::fwrite (s, sizeof (T), length, os_);
                return *this;
            }

            void flush ();

        private:
            FILE * os_;
        };

        template <typename T>
        crude_ostream & operator<< (crude_ostream & os, T const & t) {
            return os.write (t);
        }

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_OSTREAM_HPP
