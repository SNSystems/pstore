//*      _        _                           _   _      *
//*  ___| |_ __ _| |_ _   _ ___   _ __   __ _| |_| |__   *
//* / __| __/ _` | __| | | / __| | '_ \ / _` | __| '_ \  *
//* \__ \ || (_| | |_| |_| \__ \ | |_) | (_| | |_| | | | *
//* |___/\__\__,_|\__|\__,_|___/ | .__/ \__,_|\__|_| |_| *
//*                              |_|                     *
//===- lib/broker_intf/status_path.cpp ------------------------------------===//
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
#include "pstore/broker_intf/status_path.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include "pstore/config/config.hpp"
#include "pstore/support/file.hpp"
#include "pstore/support/logging.hpp"
#include "pstore/support/path.hpp"

namespace {
    constexpr std::size_t port_file_size = 8;
    char const * default_status_name = PSTORE_VENDOR_ID ".pstore_broker.status";
} // end anonymous namespace

namespace pstore {
    namespace broker {

        std::string get_status_path () {
            return path::join (file::file_handle::get_temporary_directory (), default_status_name);
        }

        void write_port_number_file (std::string const & path, in_port_t port) {
            file::unlink (path, true /*allow noent*/);
            log (logging::priority::info, "writing port number to path:", path);

            file::file_handle port_file {path};
            port_file.open (file::file_handle::create_mode::open_always,
                            file::file_handle::writable_mode::read_write);
            port_file.seek (0);
            char str[port_file_size + 1];
            std::snprintf (str, port_file_size + 1, "port%04x", port);
            assert (str[port_file_size] == '\0' && std::strlen (str) == port_file_size);
            port_file.write_span (gsl::make_span (str, port_file_size));
            port_file.truncate (port_file_size);
            port_file.close ();
        }

        in_port_t read_port_number_file (std::string const & path) {
            auto result = static_cast<in_port_t> (0);

            file::file_handle port_file {path};
            port_file.open (file::file_handle::create_mode::open_existing,
                            file::file_handle::writable_mode::read_only);
            if (port_file.size () == port_file_size) {
                char str[port_file_size + 1] = {0};
                port_file.seek (0);
                port_file.read_span (gsl::make_span (str, port_file_size));

                unsigned i = 0;
                assert (str[port_file_size] == '\0');
                if (std::sscanf (str, "port%x", &i) == 1) {
                    result = static_cast<in_port_t> (i);
                }
            }
            return result;
        }

    } // namespace broker
} // namespace pstore
