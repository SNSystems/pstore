//*                                       __ _ _                                    *
//*  _ __  _ __ ___   ___ ___  ___ ___   / _(_) | ___   _ __   __ _ _ __ ___   ___  *
//* | '_ \| '__/ _ \ / __/ _ \/ __/ __| | |_| | |/ _ \ | '_ \ / _` | '_ ` _ \ / _ \ *
//* | |_) | | | (_) | (_|  __/\__ \__ \ |  _| | |  __/ | | | | (_| | | | | | |  __/ *
//* | .__/|_|  \___/ \___\___||___/___/ |_| |_|_|\___| |_| |_|\__,_|_| |_| |_|\___| *
//* |_|                                                                             *
//===- lib/pstore_support/process_file_name_win32.cpp ---------------------===//
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
/// \file process_file_name_win32.cpp

#include "pstore_support/process_file_name.hpp"

#if defined(_WIN32)

#include <limits>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "pstore_support/portab.hpp"
#include "pstore_support/small_vector.hpp"
#include "pstore_support/utf.hpp"

namespace {

    DWORD get_module_file_name (pstore::small_vector<wchar_t> & buffer) {
        assert (buffer.size () <= std::numeric_limits<DWORD>::max ());
        DWORD const num_wchars =
            ::GetModuleFileNameW (nullptr, buffer.data (), static_cast<DWORD> (buffer.size ()));
        if (num_wchars == 0) {
            // Something went wrong.
            DWORD const last_error = ::GetLastError ();
            raise (::pstore::win32_erc (last_error), "GetModuleFileName");
        }
        return num_wchars;
    }

} // end anonymous namespace

namespace pstore {

    std::string process_file_name () {
        pstore::small_vector<wchar_t> file_name;
        file_name.resize (file_name.capacity ());
        DWORD num_wchars = get_module_file_name (file_name);
        if (num_wchars > file_name.size () - 1) {
            // We need a bigger buffer to hold the UTF-16 string. Resize and try again.
            file_name.resize (num_wchars);
            num_wchars = get_module_file_name (file_name);
            if (num_wchars != file_name.size ()) {
                raise (std::errc::no_buffer_space, "GetModuleFileName");
            }
        }

        file_name[num_wchars] = '\0';
        return {pstore::utf::win32::to8 (file_name.data ())};
    }

} // end namespace pstore

#endif // _WIN32
// eof: lib/pstore_support/process_file_name_win32.cpp
