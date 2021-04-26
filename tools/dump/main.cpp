//===- tools/dump/main.cpp ------------------------------------------------===//
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
#include <system_error>
#include <vector>

#include "pstore/config/config.hpp"

#ifdef PSTORE_IS_INSIDE_LLVM
#    include "llvm/Support/Signals.h"
#    include "llvm/Support/TargetSelect.h"
#    include "llvm/Support/PrettyStackTrace.h"
#    include "llvm/Support/ManagedStatic.h"
#    include "llvm/ADT/StringRef.h"
#endif

#include "pstore/command_line/tchar.hpp"
#include "pstore/core/generation_iterator.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/dump/db_value.hpp"
#include "pstore/dump/index_value.hpp"
#include "pstore/dump/mcdebugline_value.hpp"
#include "pstore/dump/mcrepo_value.hpp"
#include "pstore/dump/value.hpp"

#include "switches.hpp"

namespace {

    enum class dump_error_code : int {
        bad_digest = 1,
        no_fragment_index,
        fragment_not_found,
        no_compilation_index,
        compilation_not_found,
        debug_line_header_not_found,
        no_debug_line_header_index,
    };

    class dump_error_category final : public std::error_category {
    public:
        // The need for this constructor was removed by CWG defect 253 but Clang (prior to 3.9.0)
        // and GCC (before 4.6.4) require its presence.
        dump_error_category () noexcept {}
        char const * name () const noexcept override;
        std::string message (int error) const override;
    };

    char const * dump_error_category::name () const noexcept { return "pstore-dump category"; }

    std::string dump_error_category::message (int error) const {
        auto * result = "unknown error";
        switch (static_cast<dump_error_code> (error)) {
        case dump_error_code::bad_digest: result = "bad digest"; break;
        case dump_error_code::no_fragment_index: result = "no fragment index"; break;
        case dump_error_code::fragment_not_found: result = "fragment not found"; break;
        case dump_error_code::no_compilation_index: result = "no compilation index"; break;
        case dump_error_code::compilation_not_found: result = "compilation not found"; break;
        case dump_error_code::debug_line_header_not_found:
            result = "debug line header not found";
            break;
        case dump_error_code::no_debug_line_header_index:
            result = "no debug line header index";
            break;
        }
        return result;
    }

    std::error_code make_error_code (dump_error_code e) {
        static_assert (std::is_same<std::underlying_type<decltype (e)>::type, int>::value,
                       "base type of error_code must be int to permit safe static cast");

        static dump_error_category const cat;
        return {static_cast<int> (e), cat};
    }

} // end anonymous namespace

namespace std {

    template <>
    struct is_error_code_enum<dump_error_code> : std::true_type {};

} // end namespace std

namespace {

    template <typename Index>
    auto make_index (char const * name, pstore::database const & db, Index const & index)
        -> pstore::dump::value_ptr {
        using namespace pstore::dump;
        array::container members;
        auto r = index.make_range (db);
        typename Index::const_iterator b = r.begin ();
        typename Index::const_iterator e = r.end ();
        for (auto const & kvp : index.make_range (db)) {
            members.push_back (make_value (object::container{{"key", make_value (kvp.first)},
                                                             {"value", make_value (kvp.second)}}));
        }

        return make_value (object::container{
            {"name", make_value (name)},
            {"members", make_value (members)},
        });
    }

    pstore::dump::value_ptr make_name_index (pstore::database const & db) {
        using namespace pstore::dump;
        constexpr bool create = true;

        auto names = pstore::index::get_index<pstore::trailer::indices::name> (db, create);
        return make_value (names->begin (db), names->end (db));
    }

    pstore::dump::value_ptr make_path_index (pstore::database const & db) {
        using namespace pstore::dump;
        constexpr bool create = true;

        auto paths = pstore::index::get_index<pstore::trailer::indices::path> (db, create);
        return make_value (paths->begin (db), paths->end (db));
    }

    pstore::dump::value_ptr make_indices (pstore::database const & db) {
        using namespace pstore::dump;
        constexpr bool create = false;

        array::container result;
        if (std::shared_ptr<pstore::index::compilation_index const> const compilation =
                pstore::index::get_index<pstore::trailer::indices::compilation> (db, create)) {
            result.push_back (make_value (object::container{
                {"name", make_value ("compilation")},
                {"members", make_value (compilation->begin (db), compilation->end (db))},
            }));
        }

        if (std::shared_ptr<pstore::index::debug_line_header_index const> const dlh =
                pstore::index::get_index<pstore::trailer::indices::debug_line_header> (db,
                                                                                       create)) {
            result.push_back (make_value (object::container{
                {"name", make_value ("debug_line_header")},
                {"members", make_value (dlh->begin (db), dlh->end (db))},
            }));
        }

        if (std::shared_ptr<pstore::index::fragment_index const> const fragment =
                pstore::index::get_index<pstore::trailer::indices::fragment> (db, create)) {
            result.push_back (make_value (object::container{
                {"name", make_value ("fragment")},
                {"members", make_value (fragment->begin (db), fragment->end (db))},
            }));
        }

        if (std::shared_ptr<pstore::index::name_index const> const name =
                pstore::index::get_index<pstore::trailer::indices::name> (db, create)) {
            result.push_back (make_value (object::container{
                {"name", make_value ("name")},
                {"members", make_value (name->begin (db), name->end (db))},
            }));
        }

        if (std::shared_ptr<pstore::index::path_index const> const path =
                pstore::index::get_index<pstore::trailer::indices::path> (db, create)) {
            result.push_back (make_value (object::container{
                {"path", make_value ("path")},
                {"members", make_value (path->begin (db), path->end (db))},
            }));
        }

        if (std::shared_ptr<pstore::index::write_index const> const write =
                pstore::index::get_index<pstore::trailer::indices::write> (db, create)) {
            result.push_back (make_index ("write", db, *write));
        }

        return make_value (result);
    }

    pstore::dump::value_ptr make_log (pstore::dump::parameters const & parm) {
        using namespace pstore::dump;

        array::container array;
        for (pstore::typed_address<pstore::trailer> footer_pos :
             pstore::generation_container (parm.db)) {
            auto footer = parm.db.getro (footer_pos);
            auto revision = std::make_shared<object> (object::container{
                {"number", make_value (footer->a.generation.load ())},
                {"size", make_number (footer->a.size.load ())},
                {"time", make_time (footer->a.time, parm.no_times)},
            });
            revision->compact (true);
            array.emplace_back (revision);
        }
        return make_value (array);
    }

    pstore::dump::value_ptr make_shared_memory (pstore::database const & db, bool no_times) {
        (void) no_times;
        using namespace pstore::dump;

        // Shared memory is not used except on Windows.
        object::container result;
        result.emplace_back ("name", make_value (db.shared_memory_name ()));
#ifdef _WIN32
        pstore::shared const * const ptr = db.get_shared ();
        result.emplace_back ("pid", make_number (ptr->pid.load ()));
        result.emplace_back ("time", make_time (ptr->time.load (), no_times));
        result.emplace_back ("open_tick", make_number (ptr->open_tick.load ()));
#endif
        return make_value (result);
    }


    template <dump_error_code NotFoundError, typename IndexType, typename RecordFunction>
    pstore::dump::value_ptr add_specified (pstore::database const & db, IndexType const & index,
                                           std::list<pstore::index::digest> const & items_to_show,
                                           RecordFunction record_function) {
        pstore::dump::array::container container;
        container.reserve (items_to_show.size ());

        auto end = index.end (db);
        for (pstore::index::digest const & t : items_to_show) {
            auto pos = index.find (db, t);
            if (pos == end) {
                pstore::raise_error_code (make_error_code (NotFoundError));
            } else {
                container.emplace_back (record_function (*pos));
            }
        }

        return pstore::dump::make_value (container);
    }

    pstore::gsl::czstring index_to_string (pstore::trailer::indices kind) noexcept {
        char const * name = "*unknown*";
#define X(k)                                                                                       \
    case pstore::trailer::indices::k: name = #k "s"; break;

        switch (kind) {
            PSTORE_INDICES
        case pstore::trailer::indices::last: PSTORE_ASSERT (false); break;
        }
#undef X
        return name;
    }

    template <typename pstore::trailer::indices Index, dump_error_code NotFoundError,
              dump_error_code NoIndex, typename RecordFunction>
    void show_index (pstore::dump::object::container & file, pstore::database const & db,
                     bool show_all, std::list<pstore::index::digest> const & digests,
                     RecordFunction record_function) {

        if (show_all) {
            file.emplace_back (index_to_string (Index),
                               pstore::dump::make_index<Index> (db, record_function));
            return;
        }

        if (digests.size () > 0) {
            if (auto const index = pstore::index::get_index<Index> (db, false)) {
                file.emplace_back (
                    index_to_string (Index),
                    add_specified<NotFoundError> (db, *index, digests, record_function));
            } else {
                pstore::raise_error_code (make_error_code (NoIndex));
            }
        }
    }

#if defined(PSTORE_IS_INSIDE_LLVM) && defined(_WIN32) && defined(_UNICODE)
    std::pair<std::vector<std::string>, std::vector<char const *>> make_mbcs_argv (int argc,
                                                                                   TCHAR * argv[]) {
        std::vector<std::string> argv_strings;
        std::vector<char const *> argv2;
        argv_strings.reserve (argc);
        std::transform (argv, argv + argc, std::back_inserter (argv_strings), [] (TCHAR * arg) {
            return pstore::utf::win32::to_mbcs (arg, std::wcslen (arg));
        });
        argv2.reserve (argc + 1);
        std::transform (std::begin (argv_strings), std::end (argv_strings),
                        std::back_inserter (argv2),
                        [] (std::string const & s) { return s.data (); });
        argv2.emplace_back (nullptr);
        return {std::move (argv_strings), std::move (argv2)};
    }
#endif // PSTORE_IS_INSIDE_LLVM && _WIN32 && _UNICODE

    void initialize (int argc, pstore::command_line::tchar * argv[]) {
#ifdef PSTORE_IS_INSIDE_LLVM

#    if defined(_WIN32) && defined(_UNICODE)
        // Windows will present our _tmain function with its arguments encoded as UTF-16. The LLVM
        // APIs, are expecting multi-byte characters instead. That means that we
        // need to convert the encoding.
        auto const mbcs_argv = make_mbcs_argv (argc, argv);
        llvm::sys::PrintStackTraceOnErrorSignal (std::get<1> (mbcs_argv).front ());
        llvm::PrettyStackTraceProgram X (argc, std::get<1> (mbcs_argv).data ());
#    else
        llvm::sys::PrintStackTraceOnErrorSignal (argv[0]);
        llvm::PrettyStackTraceProgram X (argc, argv);
#    endif // defined(_WIN32) && defined(_UNICODE)

        llvm::llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.

        // Initialize targets and assembly printers/parsers.
        llvm::InitializeAllTargetInfos ();
        llvm::InitializeAllTargetMCs ();
        llvm::InitializeAllDisassemblers ();

#else

        (void) argc;
        (void) argv;

#endif // PSTORE_IS_INSIDE_LLVM
    }

} // end anonymous namespace


#if defined(_WIN32)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    PSTORE_TRY {
        initialize (argc, argv);

        switches opt;
        std::tie (opt, exit_code) = get_switches (argc, argv);
        if (exit_code != EXIT_SUCCESS) {
            return exit_code;
        }

        if (opt.hex) {
            pstore::dump::number_base::hex ();
        } else {
            pstore::dump::number_base::dec ();
        }
        pstore::dump::address::set_expanded (opt.expanded_addresses);

        using pstore::dump::make_value;
        using pstore::dump::object;

        pstore::dump::array::container output;
        for (std::string const & path : opt.paths) {
            pstore::database db (path, pstore::database::access_mode::read_only);

            db.sync (opt.revision);

            object::container file;
            file.emplace_back ("file",
                               make_value (object::container{{"path", make_value (path)},
                                                             {"size", make_value (db.size ())}}));

            pstore::dump::parameters parm{
                db, opt.hex, opt.expanded_addresses, opt.no_times, opt.no_disassembly, opt.triple};

            show_index<pstore::trailer::indices::fragment, dump_error_code::fragment_not_found,
                       dump_error_code::no_fragment_index> (
                file, db, opt.show_all_fragments, opt.fragments,
                [&parm] (pstore::index::fragment_index::value_type const & value) {
                    return make_value (value, parm);
                });

            show_index<pstore::trailer::indices::compilation,
                       dump_error_code::compilation_not_found,
                       dump_error_code::no_compilation_index> (
                file, db, opt.show_all_compilations, opt.compilations,
                [&parm] (pstore::index::compilation_index::value_type const & value) {
                    return make_value (value, parm);
                });

            show_index<pstore::trailer::indices::debug_line_header,
                       dump_error_code::debug_line_header_not_found,
                       dump_error_code::no_debug_line_header_index> (
                file, db, opt.show_all_debug_line_headers, opt.debug_line_headers,
                [&parm] (pstore::index::debug_line_header_index::value_type const & value) {
                    return make_value (value, parm);
                });

            if (opt.show_names) {
                file.emplace_back ("names", make_name_index (db));
            }
            if (opt.show_paths) {
                file.emplace_back ("paths", make_path_index (db));
            }

            if (opt.show_header) {
                auto header = db.getro (pstore::typed_address<pstore::header>::null ());
                file.emplace_back ("header", make_value (*header));
            }
            if (opt.show_indices) {
                file.emplace_back ("indices", make_indices (db));
            }
            if (opt.show_log) {
                file.emplace_back ("log", make_log (parm));
            }
            if (opt.show_shared) {
                file.emplace_back ("shared_memory", make_shared_memory (db, opt.no_times));
            }

            output.push_back (make_value (file));
        }

        pstore::dump::value_ptr v = make_value (output);
        pstore::command_line::out_stream << NATIVE_TEXT ("---\n") << *v << NATIVE_TEXT ("\n...\n");
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, {
        pstore::command_line::error_stream << NATIVE_TEXT ("Error: ")
                                       << pstore::utf::to_native_string (ex.what ())
                                       << std::endl;
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        pstore::command_line::error_stream << NATIVE_TEXT ("Unknown error.") << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format on
    return exit_code;
}
