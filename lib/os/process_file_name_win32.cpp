//*                                       __ _ _                                    *
//*  _ __  _ __ ___   ___ ___  ___ ___   / _(_) | ___   _ __   __ _ _ __ ___   ___  *
//* | '_ \| '__/ _ \ / __/ _ \/ __/ __| | |_| | |/ _ \ | '_ \ / _` | '_ ` _ \ / _ \ *
//* | |_) | | | (_) | (_|  __/\__ \__ \ |  _| | |  __/ | | | | (_| | | | | | |  __/ *
//* | .__/|_|  \___/ \___\___||___/___/ |_| |_|_|\___| |_| |_|\__,_|_| |_| |_|\___| *
//* |_|                                                                             *
//===- lib/os/process_file_name_win32.cpp ---------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file process_file_name_win32.cpp

#include "pstore/os/process_file_name.hpp"

#if defined(_WIN32)

#    include <limits>

#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>

#    include "pstore/adt/small_vector.hpp"
#    include "pstore/support/portab.hpp"
#    include "pstore/support/utf.hpp"

namespace {

    DWORD get_module_file_name (pstore::small_vector<wchar_t> & buffer) {
        PSTORE_ASSERT (buffer.size () <= std::numeric_limits<DWORD>::max ());
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
