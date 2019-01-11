//*                         *
//*   ___ ___  _ __  _   _  *
//*  / __/ _ \| '_ \| | | | *
//* | (_| (_) | |_) | |_| | *
//*  \___\___/| .__/ \__, | *
//*           |_|    |___/  *
//===- tools/genromfs/copy.cpp --------------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
//===----------------------------------------------------------------------===//
#include "copy.hpp"

#include <cstdio>

#include "pstore/support/array_elements.hpp"
#include "pstore/support/utf.hpp"
#include "./indent.hpp"

namespace {

#ifdef _WIN32
    FILE * file_open (std::string const & filename) {
        return _wfopen (pstore::utf::win32::to16 (filename).c_str (), L"r");
    }
#else
    FILE * file_open (std::string const & filename) { return std::fopen (filename.c_str (), "r"); }
#endif

} // end anonymous namespace

void copy (std::string const & path, unsigned file_no) {
    constexpr std::size_t indent_size = pstore::array_elements (indent) - 1U;
    constexpr auto line_width = std::size_t{80} - indent_size;
    auto getcr = [](std::size_t width) {
        return width >= line_width ? std::make_pair (std::size_t{0}, crindent)
                                   : std::make_pair (width, "");
    };

    constexpr auto buffer_size = std::size_t{1024};
    std::uint8_t buffer[buffer_size] = {0};
    std::unique_ptr<FILE, decltype (&std::fclose)> file (file_open (path), &std::fclose);
    if (!file) {
        // FIXME: report an error
        return;
    }
    std::fprintf (stdout, "std::uint8_t const file%u[] = {\n%s", file_no, indent);
    std::size_t width = indent_size;
    char const * separator = "";
    auto num_read = std::size_t{0};
    do {
        num_read = std::fread (&buffer[0], sizeof (buffer[0]), buffer_size, file.get ());
        num_read = std::min (buffer_size, num_read);
        if (std::ferror (file.get ())) {
            // FIXME: report an error
            return;
        }
        for (auto n = std::size_t{0}; n < num_read; ++n) {
            char const * cr;
            std::tie (width, cr) = getcr (width);
            int const written =
                std::fprintf (stdout, "%s%s%u", separator, cr, static_cast<unsigned> (buffer[n]));
            if (written < 0) {
                // FIXME: report an error
                return;
            }
            width += static_cast<std::make_unsigned<decltype (written)>::type> (written);
            separator = ",";
        }
    } while (num_read >= buffer_size);
    std::fprintf (stdout, "\n};\n");
}
