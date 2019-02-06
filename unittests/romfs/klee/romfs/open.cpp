#include <cassert>
#include <cinttypes>
#include <cstdio>

#ifdef PSTORE_KLEE_RUN
#    include <iostream>
#endif

#include <klee/klee.h>

#include "pstore/romfs/romfs.hpp"

using namespace pstore::romfs;

#include <system_error>
namespace std {
    error_category::error_category () noexcept = default;
    error_category::~error_category () noexcept = default;
    error_condition error_category::default_error_condition (int ev) const noexcept {
        return error_condition (ev, *this);
    }
    bool error_category::equivalent (int code, const error_condition & condition) const noexcept {
        return default_error_condition (code) == condition;
    }
    bool error_category::equivalent (const error_code & code, int condition) const noexcept {
        return *this == code.category () && code.value () == condition;
    }

    // template <typename T>
    // std::allocator<T>::allocator() noexcept = default;

    template class allocator<char>;
    template class basic_string<char>;

    void terminate () noexcept { std::exit (EXIT_FAILURE); }
} // namespace std

namespace {

    std::uint8_t const file1[] = {0};
    extern directory const dir0;
    extern directory const dir3;
    std::array<dirent, 3> const dir0_membs = {{
        {".", &dir0},
        {"..", &dir3},
        {"f", file1, sizeof (file1), 0},
    }};
    directory const dir0{dir0_membs};
    std::array<dirent, 4> const dir3_membs = {{
        {".", &dir3},
        {"..", &dir3},
        {"d", &dir0},
        {"g", file1, sizeof (file1), 0},
    }};
    directory const dir3{dir3_membs};
    directory const * const root = &dir3;

} // end anonymous namespace

int main (int argc, char * argv[]) {
    constexpr auto buffer_size = 20U;
    char path[buffer_size];
    klee_make_symbolic (&path, sizeof (path), "path");
    klee_assume (path[buffer_size - 1U] == '\0');

#ifdef PSTORE_KLEE_RUN
    std::cout << path << '\n';
#endif

    romfs fs (root);
    fs.chdir ("d");
    fs.open (path);
}
