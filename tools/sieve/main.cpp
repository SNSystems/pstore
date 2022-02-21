//===- tools/sieve/main.cpp -----------------------------------------------===//
//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include <cstdint>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// pstore includes
#include "pstore/support/error.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/quoted.hpp"
#include "pstore/support/utf.hpp"

// Local includes
#include "switches.hpp"
#include "write_output.hpp"

namespace {

    template <typename T, typename R>
    struct check_range {
        void operator() (R value) const {
            (void) value;
            PSTORE_ASSERT (value <= std::numeric_limits<T>::max ());
        }
    };
    template <typename T>
    struct check_range<T, T> {
        void operator() (T) const {}
    };


    template <typename IntType>
    std::vector<IntType> sieve (std::uint64_t top_value) {
        check_range<IntType, decltype (top_value)>{}(top_value);

        std::vector<IntType> result;
        result.push_back (1);
        result.push_back (2);

        std::vector<bool> is_prime ((top_value + 1) / 2 /*count*/, 1 /*value*/);
        for (auto ctr = 3UL; ctr <= top_value; ctr += 2) {
            if (is_prime[ctr / 2]) {
                result.push_back (static_cast<IntType> (ctr));

                for (auto multiple = ctr * ctr; multiple <= top_value; multiple += 2 * ctr) {
                    is_prime[multiple / 2] = 0;
                }
            }
        }
        return result;
    }

    //*   __ _ _                                   *
    //*  / _(_) |___   ___ _ __  ___ _ _  ___ _ _  *
    //* |  _| | / -_) / _ \ '_ \/ -_) ' \/ -_) '_| *
    //* |_| |_|_\___| \___/ .__/\___|_||_\___|_|   *
    //*                   |_|                      *
    class file_opener {
    public:
        explicit file_opener (std::string const & path);
        file_opener (file_opener const &) = delete;

        file_opener & operator= (file_opener const &) = delete;
        file_opener & operator= (file_opener &&) = delete;

        std::ostream & get_stream () const noexcept { return *out_; }

    private:
        std::unique_ptr<std::ofstream> file_;
        std::ostream * out_ = nullptr;

        static std::unique_ptr<std::ofstream> open_file (std::string const & path);
    };

    // (ctor)
    // ~~~~~~
    file_opener::file_opener (std::string const & path) {
        if (path == "-") {
            out_ = &std::cout;
            return;
        }
        file_ = file_opener::open_file (path);
        out_ = file_.get ();
    }

    // open file
    // ~~~~~~~~~
    std::unique_ptr<std::ofstream> file_opener::open_file (std::string const & path) {
        auto file = std::make_unique<std::ofstream> (
            path, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
        if (!file->is_open ()) {
            std::ostringstream str;
            str << "Could not open " << pstore::quoted (path);
            pstore::raise (std::errc::no_such_file_or_directory, str.str ());
        }
        return file;
    }

} // end anonymous namespace


#if defined(_WIN32)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    PSTORE_TRY {
        user_options const opt = user_options::get (argc, argv);

        file_opener const out{opt.output};
        if (opt.maximum <= std::numeric_limits<std::uint16_t>::max ()) {
            write_output (sieve<std::uint16_t> (opt.maximum), opt.endianness, out.get_stream ());
        } else if (opt.maximum <= std::numeric_limits<std::uint32_t>::max ()) {
            write_output (sieve<std::uint32_t> (opt.maximum), opt.endianness, out.get_stream ());
        } else {
            write_output (sieve<std::uint64_t> (opt.maximum), opt.endianness, out.get_stream ());
        }
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, {
        pstore::command_line::error_stream << PSTORE_NATIVE_TEXT ("An error occurred: ")
                                       << pstore::utf::to_native_string (ex.what ())
                                       << std::endl;
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        pstore::command_line::error_stream << PSTORE_NATIVE_TEXT ("Unknown exception") << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format on

    return exit_code;
}
