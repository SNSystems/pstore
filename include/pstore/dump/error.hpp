//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===- include/pstore/dump/error.hpp --------------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_DUMP_ERROR_HPP
#define PSTORE_DUMP_ERROR_HPP

#include <string>
#include <system_error>

namespace pstore {
    namespace dump {

        enum class error_code : int {
            cant_find_target = 1,
            no_register_info_for_target,
            no_assembly_info_for_target,
            no_subtarget_info_for_target,
            no_instruction_info_for_target,
            no_disassembler_for_target,
            no_instruction_printer,
            cannot_create_asm_streamer,
            unknown_target,
        };

        class error_category : public std::error_category {
        public:
            // The need for this constructor was removed by CWG defect 253 but Clang (prior
            // to 3.9.0) and GCC (before 4.6.4) require its presence.
            error_category () noexcept {}
            char const * name () const noexcept override;
            std::string message (int error) const override;
        };

        std::error_code make_error_code (error_code e);

    } // end namespace dump
} // end namespace pstore

namespace std {

    template <>
    struct is_error_code_enum<pstore::dump::error_code> : std::true_type {};

} // end namespace std

#endif // PSTORE_DUMP_ERROR_HPP
