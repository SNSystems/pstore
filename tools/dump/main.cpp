//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===- tools/dump/main.cpp ------------------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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

#include "dump/db_value.hpp"
#include "dump/value.hpp"
#include "dump/mcrepo_value.hpp"
#include "dump_config.hpp"

#if PSTORE_IS_INSIDE_LLVM
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/ADT/StringRef.h"
#endif

#include "pstore/database.hpp"
#include "pstore/generation_iterator.hpp"
#include "pstore/hamt_map.hpp"
#include "pstore/hamt_set.hpp"
#include "pstore/index_types.hpp"
#include "pstore/shared_memory.hpp"
#include "pstore/sstring_view_archive.hpp"
#include "pstore/vacuum_intf.hpp"
#include "pstore_cmd_util/str_to_revision.hpp"
#include "pstore_support/error.hpp"
#include "pstore_support/portab.hpp"
#include "pstore_support/utf.hpp"

#include "switches.hpp"

namespace {

    enum class dump_error_code {
        bad_digest = 1,
        no_digest_index,
        fragment_not_found,
        bad_uuid,
        no_ticket_index,
        ticket_not_found,
        bad_ticket_file,
    };

    class dump_error_category : public std::error_category {
    public:
        dump_error_category () noexcept = default;
        char const * name () const noexcept override;
        std::string message (int error) const override;
    };

    char const * dump_error_category::name () const noexcept {
        return "pstore-dump category";
    }

    std::string dump_error_category::message (int error) const {
        switch (static_cast<dump_error_code> (error)) {
        case dump_error_code::bad_digest:
            return "bad digest";
        case dump_error_code::no_digest_index:
            return "no digest index";
        case dump_error_code::fragment_not_found:
            return "fragment not found";
        case dump_error_code::bad_uuid:
            return "bad UUID";
        case dump_error_code::no_ticket_index:
            return "no ticket index";
        case dump_error_code::ticket_not_found:
            return "ticket not found";
        case dump_error_code::bad_ticket_file:
            return "bad ticket file";
        }
        return "unknown error";
    }

    std::error_category const & get_dump_error_category () {
        static dump_error_category const cat;
        return cat;
    }

} // end anonymous namespace

namespace std {

    template <>
    struct is_error_code_enum<dump_error_code> : public std::true_type {};

    std::error_code make_error_code (dump_error_code e) {
        static_assert (std::is_same<std::underlying_type<decltype (e)>::type, int>::value,
                       "base type of error_code must be int to permit safe static cast");
        return {static_cast<int> (e), get_dump_error_category ()};
    }

} // namespace std

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
        if (pstore::index::write_index * const write =
                pstore::index::get_write_index (db, false /* create*/)) {
            result.push_back (make_index ("write", *write));
        }
        if (pstore::index::name_index * const name =
                pstore::index::get_name_index (db, false /* create */)) {
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
            auto revision = std::make_shared<value::object> (value::object::container{
                {"number", value::make_value (footer->a.generation.load ())},
                {"size", value::make_number (footer->a.size.load ())},
                {"time", value::make_time (footer->a.time, no_times)},
            });
            revision->compact (true);
            array.emplace_back (revision);
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

    unsigned hex_to_digit (char digit) {
        if (digit >= 'a' && digit <= 'f') {
            return static_cast<unsigned> (digit) - ('a' - 10);
        }
        if (digit >= 'A' && digit <= 'F') {
            return static_cast<unsigned> (digit) - ('A' - 10);
        }
        if (digit >= '0' && digit <= '9') {
            return static_cast<unsigned> (digit) - '0';
        }
        pstore::raise_error_code (std::make_error_code (dump_error_code::bad_digest));
    }

    pstore::index::digest string_to_digest (std::string const & str) {
        if (str.length () != 32) {
            pstore::raise_error_code (std::make_error_code (dump_error_code::bad_digest));
        }
        auto get64 = [&str](int index) {
            assert (index >= 0 && static_cast<unsigned> (index) < str.length ());
            auto result = std::uint64_t{0};
            for (auto shift = 60; shift >= 0; shift -= 4, ++index) {
                result |= (static_cast<std::uint64_t> (hex_to_digit (str[index])) << shift);
            }
            assert (index >= 0);
            return result;
        };
        return {get64 (0), get64 (16)};
    }



    // FIXME: this is duplicated in the LLVM code.
    struct ticket_file_contents {
        std::array<std::uint8_t, 8> signature;
        pstore::uuid id;
    };
    constexpr std::array<std::uint8_t, 8> ticket_signature{
        {'R', 'e', 'p', 'o', 'U', 'u', 'i', 'd'}};
    constexpr std::uint64_t ticket_file_size = 24;
    static_assert (sizeof (ticket_file_contents) == ticket_file_size,
                   "ticket_file_contents must match ticket_file_size");

    pstore::uuid string_to_ticket (std::string const & str) {
        pstore::maybe<pstore::uuid> uuid = pstore::uuid::from_string (str);
        if (uuid.has_value ()) {
            return *uuid;
        }

        pstore::file::file_handle f;
        f.open (str, pstore::file::file_handle::create_mode::open_existing,
                pstore::file::file_handle::writable_mode::read_only);
        if (f.is_open () && f.size () == ticket_file_size) {
            ticket_file_contents contents;
            f.read (&contents);
            if (contents.signature == ticket_signature) {
                return contents.id;
            }
        }

        pstore::raise_error_code (std::make_error_code (dump_error_code::bad_ticket_file), str);
    }

    template <typename IndexType, typename StringToKeyFunction, typename RecordFunction>
    value::value_ptr
    add_specified (IndexType const & index, std::list<std::string> const & items_to_show,
                   dump_error_code not_found_error, StringToKeyFunction string_to_key,
                   RecordFunction record_function) {
        value::array::container container;
        container.reserve (items_to_show.size ());

        auto end = std::end (index);
        for (std::string const & t : items_to_show) {
            auto const key = string_to_key (t);
            auto pos = index.find (key);
            if (pos == end) {
                pstore::raise_error_code (std::make_error_code (not_found_error));
            } else {
                container.emplace_back (record_function (*pos));
            }
        }

        return value::make_value (container);
    }

} // end anonymous namespace

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
#if PSTORE_IS_INSIDE_LLVM
        llvm::sys::PrintStackTraceOnErrorSignal (argv[0]);
        llvm::PrettyStackTraceProgram X (argc, argv);
        llvm::llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.

        // Initialize targets and assembly printers/parsers.
        llvm::InitializeAllTargetInfos ();
        llvm::InitializeAllTargetMCs ();
        llvm::InitializeAllDisassemblers ();
#endif

        switches opt;
        std::tie (opt, exit_code) = get_switches (argc, argv);
        if (exit_code != EXIT_SUCCESS) {
            return exit_code;
        }

        bool show_contents = opt.show_contents;
        bool show_all_fragments = opt.show_all_fragments;
        bool show_all_tickets = opt.show_all_tickets;
        bool show_header = opt.show_header;
        bool show_indices = opt.show_indices;
        bool show_log = opt.show_log;
        bool show_shared = opt.show_shared;
        if (opt.show_all) {
            show_contents = show_all_fragments = show_all_tickets = show_header = show_indices =
                show_log = true;
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
            if (show_all_fragments) {
                file.emplace_back ("fragments", value::make_fragments (db, opt.hex));
            } else {
                if (opt.fragments.size () > 0) {
                    if (auto * const digest_index = pstore::index::get_digest_index (db)) {
                        auto record =
                            [&db, &opt](pstore::index::digest_index::value_type const & value) {
                                auto fragment = pstore::repo::fragment::load (db, value.second);
                                return value::make_value (value::object::container{
                                    {"digest", value::make_value (value.first)},
                                    {"fragment", value::make_value (db, *fragment, opt.hex)}});
                            };

                        file.emplace_back ("fragments",
                                           add_specified (*digest_index, opt.fragments,
                                                          dump_error_code::fragment_not_found,
                                                          string_to_digest, record));
                    } else {
                        pstore::raise_error_code (std::make_error_code (dump_error_code::no_digest_index));
                    }
                }
            }

            if (show_all_tickets) {
                file.emplace_back ("tickets", value::make_tickets (db));
            } else {
                if (opt.tickets.size () > 0) {
                    if (auto * const ticket_index = pstore::index::get_ticket_index (db)) {
                        auto record = [&db](pstore::index::ticket_index::value_type const & value) {
                            auto ticket = pstore::repo::ticket::load (db, value.second);
                            return value::make_value (value::object::container{
                                {"id", value::make_value (value.first)},
                                {"ticket", value::make_value (db, ticket)}});
                        };

                        file.emplace_back ("tickets",
                                           add_specified (*ticket_index, opt.tickets,
                                                          dump_error_code::ticket_not_found,
                                                          string_to_ticket, record));
                    } else {
                        pstore::raise_error_code (
                            std::make_error_code (dump_error_code::no_ticket_index));
                    }
                }
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
