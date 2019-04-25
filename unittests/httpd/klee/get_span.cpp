#include <cstring>
#include <vector>

#include <klee/klee.h>

#include "pstore/httpd/buffered_reader.hpp"

using IO = int;
using byte_span = pstore::gsl::span<std::uint8_t>;


int main () {

    auto refill = [](IO io, byte_span const & sp) {
        std::memset (sp.data (), 0, sp.size ());
        return pstore::error_or_n<IO, byte_span::iterator>{pstore::in_place, io, sp.end ()};
    };

    std::size_t buffer_size;
    klee_make_symbolic (&buffer_size, sizeof (buffer_size), "buffer_size");
    std::size_t requested_size;
    klee_make_symbolic (&requested_size, sizeof (requested_size), "requested_size");

    klee_assume (buffer_size < 5);
    klee_assume (requested_size < 5);

    auto br = pstore::httpd::make_buffered_reader<int> (refill, buffer_size);

    std::vector<std::uint8_t> v;
    v.resize (requested_size);

    auto io = IO{};
    br.get_span (io, pstore::gsl::make_span (v));
}
