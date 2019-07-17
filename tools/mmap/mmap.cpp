
// Standard library
#include <algorithm>
#include <iostream>
#include <numeric>

// pstore
#include "pstore/os/file.hpp"
#include "pstore/os/memory_mapper.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/utf.hpp"

#ifdef _WIN32
#    define NATIVE_TEXT(str) _TEXT (str)
#else
#    define NATIVE_TEXT(str) str
#endif

namespace {

#ifdef PSTORE_EXCEPTIONS
    auto & error_stream =
#    if defined(_WIN32) && defined(_UNICODE)
        std::wcerr;
#    else
        std::cerr;
#    endif
#endif // PSTORE_EXCEPTIONS


    void write (std::size_t size) {
        pstore::file::file_handle file;
        file.open (pstore::file::file_handle::temporary ());

        file.seek (size - 1U);
        file.write (0);

        pstore::memory_mapper mm{file,  // backing file
                                 true,  // writable?
                                 0U,    // offset
                                 size}; // number of bytes to map

        // Flood the memory mapped file with values.
        auto ptr = std::static_pointer_cast<std::uint8_t> (mm.data ());
        std::iota (ptr.get (), ptr.get () + size, std::uint8_t{0});
    }

} // end anonymous namespace

int main () {
    int exit_code = EXIT_SUCCESS;
    PSTORE_TRY {
        write (1024);
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, {
        error_stream << NATIVE_TEXT ("Error: ")
                     << pstore::utf::to_native_string (ex.what ()) << std::endl;
        exit_code = EXIT_FAILURE;
    }) PSTORE_CATCH (..., {
        error_stream << NATIVE_TEXT ("Unknown error.") << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format on
    return exit_code;
}
