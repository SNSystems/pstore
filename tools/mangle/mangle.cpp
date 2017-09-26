//*                              _       *
//*  _ __ ___   __ _ _ __   __ _| | ___  *
//* | '_ ` _ \ / _` | '_ \ / _` | |/ _ \ *
//* | | | | | | (_| | | | | (_| | |  __/ *
//* |_| |_| |_|\__,_|_| |_|\__, |_|\___| *
//*                        |___/         *
//===- tools/mangle/mangle.cpp --------------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc. 
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
/// \file mangle.cpp
/// The basic technique here is based on Ilja van Sprundel's "mangle.c", although this is a
/// new implementation in C++11 and using our libraries for portability. The original code is
/// GPLed.
// FIXME: check the license is okay.
// Here's the comment from the original source file:
// https://ext4.wiki.kernel.org/index.php/Filesystem_Testing_Tools/mangle.c
//* This code is based on:
/*
  trivial binary file fuzzer by Ilja van Sprundel.
  It's usage is very simple, it takes a filename and headersize
  as input. it will then change approximatly between 0 and 10% of
  the header with random bytes (biased towards the highest bit set)

  obviously you need a bash script or something as a wrapper !

  so far this broke: - libmagic (used file)
                     - preview (osX pdf viewer)
                     - xpdf (hang, not a crash ...)
                     - mach-o loading (osX 10.3.7, seems to be fixed later)
                     - qnx elf loader (panics almost instantly, yikes !)
                     - FreeBSD elf loading
                     - openoffice
                     - amp
                     - osX image loading (.dmg)
                     - libbfd (used objdump)
                     - libtiff (used tiff2pdf)
                     - xine (division by 0, took 20 minutes of fuzzing)
                     - OpenBSD elf loading (3.7 on a sparc)
                     - unixware 713 elf loading
                     - DragonFlyBSD elf loading
                     - solaris 10 elf loading
                     - cistron-radiusd
                     - linux ext2fs (2.4.29) image loading (division by 0)
                     - linux reiserfs (2.4.29) image loading (instant panic !!!)
                     - linux jfs (2.4.29) image loading (long (uninteruptable) loop, 2 oopses)
                     - linux xfs (2.4.29) image loading (instant panic)
                     - windows macromedia flash .swf loading (obviously the windows version of
  mangle needs a few tweaks to work ...)
                     - Quicktime player 7.0.1 for MacOS X
                     - totem
                     - gnumeric
                     - vlc
                     - mplayer
                     - python bytecode interpreter
                     - realplayer 10.0.6.776 (GOLD)
                     - dvips
 */

#include <cstdint>
#include <limits>
#include <random>
#include <exception>
#include <iostream>

#include "pstore_support/file.hpp"
#include "pstore_support/portab.hpp"
#include "pstore/file_header.hpp"
#include "pstore/memory_mapper.hpp"

namespace {

    template <typename Ty>
    class random_generator {
    public:
        random_generator ()
                : device_ ()
                , generator_ (device_ ())
                , distribution_ () {}

        Ty get (Ty max) {
            return distribution_ (generator_) % max;
        }
        Ty get () {
            auto const max = std::numeric_limits<Ty>::max ();
            static_assert (max > Ty (0), "max must be > 0");
            return distribution_ (generator_) % max;
        }

    private:
        std::random_device device_;
        std::mt19937_64 generator_;
        std::uniform_int_distribution<Ty> distribution_;
    };
}

#ifdef PSTORE_CPP_EXCEPTIONS
#define TRY  try
#define CATCH(ex,proc)  catch (ex) proc
#else
#define TRY
#define CATCH(ex,proc)
#endif

int main (int argc, char ** argv) {
    int exit_code = EXIT_SUCCESS;

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " path-name\n"
                  << " \"Fuzzes\" the header and r0 footer of the given file.\n"
                  << " Warning: The file is modified in-place.\n";
        return EXIT_FAILURE;
    }

    TRY {
        random_generator<std::size_t> rand;

        auto const header_size = sizeof (pstore::header) + sizeof (pstore::trailer);

        pstore::file::file_handle file;
        file.open (argv[1], pstore::file::file_handle::create_mode::open_existing,
                   pstore::file::file_handle::writable_mode::read_write);
        pstore::memory_mapper mapper (file,
                                      true,         // writable
                                      0,            // offset
                                      header_size); // length
        auto data = std::static_pointer_cast<std::uint8_t> (mapper.data ());
        auto ptr = data.get ();

        std::size_t const num_to_hit = rand.get (header_size / 10);
        for (std::size_t ctr = 0; ctr < num_to_hit; ctr++) {
            std::size_t const offset = rand.get (header_size);
            std::uint8_t new_value = rand.get () % std::numeric_limits<std::uint8_t>::max ();

            // We want the highest bit set more often, in case of signedness issues.
            if (rand.get () % 2) {
                new_value |= 0x80;
            }

            ptr[offset] = new_value;
        }
    } CATCH (std::exception const & ex,  {
        std::cerr << "Error: " << ex.what () << std::endl;
        exit_code = EXIT_FAILURE;
    }) CATCH (..., {
        std::cerr << "Unknown error" << std::endl;
        exit_code = EXIT_FAILURE;
    })
    std::cerr << "Mangle returning " << exit_code << '\n';
    return exit_code;
}
// eof: tools/mangle/mangle.cpp
