#include "pstore/exchange/export_ostream.hpp"

#include <cinttypes>

namespace pstore {
    namespace exchange {

        crude_ostream & crude_ostream::write (char c) {
            std::fputc (c, os_);
            return *this;
        }
        crude_ostream & crude_ostream::write (std::uint16_t v) {
            std::fprintf (os_, "%" PRIu16, v);
            return *this;
        }
        crude_ostream & crude_ostream::write (std::uint32_t v) {
            std::fprintf (os_, "%" PRIu32, v);
            return *this;
        }
        crude_ostream & crude_ostream::write (std::uint64_t v) {
            std::fprintf (os_, "%" PRIu64, v);
            return *this;
        }
        crude_ostream & crude_ostream::write (gsl::czstring str) {
            std::fputs (str, os_);
            return *this;
        }
        crude_ostream & crude_ostream::write (std::string const & str) {
            std::fputs (str.c_str (), os_);
            return *this;
        }

        void crude_ostream::flush () { std::fflush (os_); }

    } // end namespace exchange
} // end namespace pstore
