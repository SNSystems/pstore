//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===- tools/dump/main.cpp ------------------------------------------------===//
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
/// \file main.cpp

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <vector>

#include "pstore/database.hpp"
#include "pstore/generation_iterator.hpp"
#include "pstore/hamt_map.hpp"
#include "pstore/hamt_set.hpp"
#include "pstore/shared_memory.hpp"
#include "pstore/vacuum_intf.hpp"
#include "pstore_cmd_util/str_to_revision.hpp"
#include "pstore_support/error.hpp"
#include "pstore_support/portab.hpp"
#include "pstore_support/utf.hpp"

#include "dump/db_value.hpp"
#include "dump/value.hpp"
#include "dump/mcrepo_value.hpp"

#include "./switches.hpp"


namespace {

    auto & out_stream =
#if defined(_WIN32) && defined(_UNICODE)
        std::wcout;
#else
        std::cout;
#endif

#ifdef PSTORE_CPP_EXCEPTIONS
    auto & error_stream =
#if defined(_WIN32) && defined(_UNICODE)
        std::wcerr;
#else
        std::cerr;
#endif
#endif // PSTORE_CPP_EXCEPTIONS

    template <typename Index>
    auto make_index (char const * name, Index const & index) -> value::value_ptr {
        value::array::container members;
        for (auto const & kvp : index) {
            members.push_back (value::make_value (
                value::object::container{{"key", value::make_value (kvp.first)},
                                         {"value", value::make_value (kvp.second)}}));
        }

        return value::make_value (value::object::container{
            {"name", value::make_value (name)}, {"members", value::make_value (members)},
        });
    }

    value::value_ptr make_indices (pstore::database & db) {
        value::array::container result;
        if (pstore::index::write_index * const write = db.get_write_index (false /* create*/)) {
            result.push_back (make_index ("write", *write));
        }
        if (pstore::index::name_index * const name = db.get_name_index (false /* create */)) {
            result.push_back (value::make_value (value::object::container{
                {"name", value::make_value ("name")},
                {"members", value::make_value (std::begin (*name), std::end (*name))},
            }));
        }
        return value::make_value (result);
    }


    value::value_ptr make_log (pstore::database & db, bool no_times) {
        value::array::container array;
        for (pstore::address footer_pos : pstore::generation_container (db)) {
            auto footer = db.getro<pstore::trailer> (footer_pos);
            array.emplace_back (value::make_value (value::object::container{
                {"number", value::make_value (footer->a.generation.load ())},
                {"time", value::make_time (footer->a.time, no_times)},
                {"size", value::make_number (footer->a.size.load ())},
            }));
        }
        return value::make_value (array);
    }

    value::value_ptr make_shared_memory (pstore::database & db, bool no_times) {
        (void) no_times;

        // Shared memory is not used except on Windows.
        value::object::container result;
        result.emplace_back ("name", value::make_value (db.shared_memory_name ()));
#ifdef _WIN32
        pstore::shared const * const ptr = db.get_shared ();
        result.emplace_back ("pid", value::make_number (ptr->pid.load ()));
        result.emplace_back ("time", value::make_time (ptr->time.load (), no_times));
        result.emplace_back ("open_tick", value::make_number (ptr->open_tick.load ()));
#endif
        return value::make_value (result);
    }


    std::uint64_t file_size (::pstore::gsl::czstring path) {
        errno = 0;
#ifdef _WIN32
        struct __stat64 buf;
        int err = _stat64 (path, &buf);
#else
        struct stat buf;
        int err = stat (path, &buf);
#endif
        if (err != 0) {
            std::ostringstream str;
            str << "Could not determine file size of \"" << path << '"';
            raise (pstore::errno_erc{errno}, str.str ());
        }

        PSTORE_STATIC_ASSERT (sizeof (buf.st_size) == sizeof (std::uint64_t));
        assert (buf.st_size >= 0);
        return static_cast<std::uint64_t> (buf.st_size);
    }
} // namespace

#ifdef PSTORE_CPP_EXCEPTIONS
#define TRY try
#define CATCH(ex, proc) catch (ex) proc
#else
#define TRY
#define CATCH(ex, proc)
#endif


#if defined(_WIN32) && !defined(PSTORE_IS_INSIDE_LLVM)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    TRY {
        switches opt;
        std::tie (opt, exit_code) = get_switches (argc, argv);
        if (exit_code != EXIT_SUCCESS) {
            return exit_code;
        }

        bool show_contents = opt.show_contents;
        bool show_fragments = opt.show_fragments;
        bool show_tickets = opt.show_tickets;
        bool show_header = opt.show_header;
        bool show_indices = opt.show_indices;
        bool show_log = opt.show_log;
        bool show_shared = opt.show_shared;
        if (opt.show_all) {
            show_contents = show_fragments = show_tickets = show_header = show_indices = show_log =
                true;
        }

        if (opt.hex) {
            value::number_base::hex ();
        } else {
            value::number_base::dec ();
        }
        value::address::set_expanded (opt.expanded_addresses);

        bool const no_times = opt.no_times;

        value::array::container output;
        for (std::string const & path : opt.paths) {
            pstore::database db (path, pstore::database::access_mode::read_only);

            db.sync (opt.revision);

            value::object::container file;
            file.emplace_back ("file",
                               value::make_value (value::object::container{
                                   {"path", value::make_value (path)},
                                   {"size", value::make_value (file_size (path.c_str ()))}}));

            if (show_contents) {
                file.emplace_back ("contents",
                                   value::make_contents (db, db.footer_pos (), no_times));
            }
            if (show_fragments) {
                file.emplace_back ("fragments", value::make_fragments (db));
            }

            if (show_tickets) {
                file.emplace_back ("tickets", value::make_tickets (db));
            }

            if (show_header) {
                auto header = db.getro<pstore::header> (pstore::address::null ());
                file.emplace_back ("header", value::make_value (*header));
            }
            if (show_indices) {
                file.emplace_back ("indices", make_indices (db));
            }
            if (show_log) {
                file.emplace_back ("log", make_log (db, no_times));
            }
            if (show_shared) {
                file.emplace_back ("shared_memory", make_shared_memory (db, no_times));
            }

            output.push_back (value::make_value (file));
        }

        value::value_ptr v = value::make_value (output);
        out_stream << NATIVE_TEXT ("---\n") << *v << NATIVE_TEXT ("\n...\n");
    } CATCH (std::exception const & ex, {
        error_stream << NATIVE_TEXT ("Error: ") << pstore::utf::to_native_string (ex.what ())
                     << std::endl;
        exit_code = EXIT_FAILURE;
    }) CATCH (..., {
        error_stream << NATIVE_TEXT ("Unknown error.") << std::endl;
        exit_code = EXIT_FAILURE;
    }) 
	return exit_code;
}

// eof: tools/dump/main.cpp
