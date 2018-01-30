//*   __                       _       *
//*  / _|_ __ __ _  __ _  __ _| | ___  *
//* | |_| '__/ _` |/ _` |/ _` | |/ _ \ *
//* |  _| | | (_| | (_| | (_| | |  __/ *
//* |_| |_|  \__,_|\__, |\__, |_|\___| *
//*                |___/ |___/         *
//===- tools/fraggle/fraggle.cpp ------------------------------------------===//
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

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "pstore/hamt_map.hpp"
#include "pstore/hamt_set.hpp"
#include "pstore/index_types.hpp"
#include "pstore/sstring_view_archive.hpp"
#include "pstore/transaction.hpp"
#include "pstore_cmd_util/cl/command_line.hpp"
#include "pstore_mcrepo/fragment.hpp"
#include "pstore_mcrepo/ticket.hpp"
#include "pstore_support/path.hpp"

#include "fibonacci_generator.hpp"

// FIXME: there are numerous definitions of these macros. Put them in portab.hpp.
// FIXME: define PSTORE_CPP_EXCEPTIONS somewhere that can be shared between the various tools.
#ifdef PSTORE_CPP_EXCEPTIONS
#define TRY try
#define CATCH(ex, proc) catch (ex) proc
#else
#define TRY
#define CATCH(ex, proc)
#endif

namespace {

    using namespace pstore::cmd_util;

    cl::opt<unsigned> num_fragments_per_ticket ("fragments",
                                                cl::desc ("Number of fragments per ticket"),
                                                cl::init (1000U));
    cl::opt<unsigned> num_tickets ("tickets", cl::desc ("Number of tickets"), cl::init (100U));
    cl::opt<std::string> output_dir ("O", cl::desc ("output directory"), cl::init ("./"));
    cl::opt<unsigned> section_size ("section-size",
                                    cl::desc ("Number of 32-bit values in the generated sections"),
                                    cl::init (16U));



    /// A version of std::copy() which returns the updated input iterator.
    template <class InputIt, class Size, class OutputIt>
    InputIt copy_n2 (InputIt first, Size count, OutputIt result) {
        for (Size ctr = 0; ctr < count; ++ctr) {
            *(result++) = *first;
            ++first;
        }
        return first;
    }
}


// FIXME: define PSTORE_IS_INSIDE_LLVM somewhere that can be shared between the various tools.
#if defined(_WIN32) && !PSTORE_IS_INSIDE_LLVM
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    TRY {
        pstore::cmd_util::cl::ParseCommandLineOptions (argc, argv, "Fragment and ticket generator");

        pstore::database db (pstore::path::join (output_dir, "clang.db"),
                             pstore::database::access_mode::writable);

        fibonacci_generator<> fib;

        // Tuple contains the address of the function name and the fragment extent.
        std::vector<pstore::repo::ticket_member> ticket_members;
        ticket_members.reserve (num_fragments_per_ticket);

        for (auto ticket_ctr = 0U; ticket_ctr < num_tickets; ++ticket_ctr) {
            auto transaction = pstore::begin (db);
            pstore::index::name_index * const names = pstore::index::get_name_index (db);
            pstore::index::digest_index * const fragment_index =
                pstore::index::get_digest_index (db);
            pstore::index::ticket_index * const ticket_index = pstore::index::get_ticket_index (db);

            ticket_members.clear ();
            for (unsigned fragment_ctr = 0; fragment_ctr < num_fragments_per_ticket;
                 ++fragment_ctr) {

                auto const name = std::string{"func_"} + std::to_string (ticket_ctr) + "_" +
                                  std::to_string (fragment_ctr);
                pstore::address const name_addr =
                    names
                        ->insert (transaction,
                                  pstore::make_sstring_view (name.data (), name.length ()))
                        .first.get_address ();

                pstore::repo::section_content data_section (pstore::repo::section_type::read_only,
                                                            std::uint8_t{1} /*alignment*/);
                std::vector<std::uint32_t> values;
                values.reserve (section_size.get ());
                fib = copy_n2 (fib, section_size.get (), std::back_inserter (values));
                for (auto v : values) {
                    for (int shift = 24; shift >= 0; shift -= 8) {
                        data_section.data.push_back ((v >> shift) & 0xFF);
                    }
                }
                assert (data_section.data.size () == section_size.get () * 4);

                pstore::extent const fragment_pos =
                    pstore::repo::fragment::alloc (transaction, &data_section, &data_section + 1);

                unsigned const digest_half = ticket_ctr * num_fragments_per_ticket + fragment_ctr;
                pstore::index::digest const digest{digest_half, digest_half};
                fragment_index->insert (transaction, std::make_pair (digest, fragment_pos));

                ticket_members.emplace_back (digest, name_addr,
                                             pstore::repo::linkage_type::external);
            }

            std::string const ticket_path =
                pstore::path::join (output_dir, "t" + std::to_string (ticket_ctr) + ".o");
            pstore::uuid ticket_uuid;
            {
                pstore::address const ticket_path_addr =
                    names
                        ->insert (transaction, pstore::make_sstring_view (ticket_path.data (),
                                                                          ticket_path.length ()))
                        .first.get_address ();
                pstore::extent ticket_pos =
                    pstore::repo::ticket::alloc (transaction, ticket_path_addr, ticket_members);
                ticket_index->insert (transaction, std::make_pair (ticket_uuid, ticket_pos));
            }

            transaction.commit ();

            {
                static std::array<char, 8> const ticket_file_signature{
                    {'R', 'e', 'p', 'o', 'U', 'u', 'i', 'd'}};
                std::ofstream ticket_file (ticket_path.c_str (),
                                           std::ios_base::out | std::ios_base::binary);
                ticket_file.write (ticket_file_signature.data (), ticket_file_signature.size ());
                ticket_file.write (reinterpret_cast<char const *> (ticket_uuid.array ().data ()),
                                   ticket_uuid.elements);
            }
        }
    }
    CATCH (std::exception const & ex, {
        std::cerr << "Error: " << ex.what () << '\n';
        exit_code = EXIT_FAILURE;
    })
    CATCH (..., {
        std::cerr << "Unknown exception\n";
        exit_code = EXIT_FAILURE;
    })

    return exit_code;
}
// eof: tools/fraggle/fraggle.cpp
