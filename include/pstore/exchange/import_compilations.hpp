//*  _                            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| *
//* | | | | | | | |_) | (_) | |  | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| *
//*             |_|                    *
//*                            _ _       _   _                  *
//*   ___ ___  _ __ ___  _ __ (_) | __ _| |_(_) ___  _ __  ___  *
//*  / __/ _ \| '_ ` _ \| '_ \| | |/ _` | __| |/ _ \| '_ \/ __| *
//* | (_| (_) | | | | | | |_) | | | (_| | |_| | (_) | | | \__ \ *
//*  \___\___/|_| |_| |_| .__/|_|_|\__,_|\__|_|\___/|_| |_|___/ *
//*                     |_|                                     *
//===- include/pstore/exchange/import_compilations.hpp --------------------===//
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
#ifndef PSTORE_EXCHANGE_IMPORT_COMPILATIONS_HPP
#define PSTORE_EXCHANGE_IMPORT_COMPILATIONS_HPP

#include "import_rule.hpp"
#include "import_names.hpp"

namespace pstore {
    namespace exchange {

        class compilations_index final : public rule {
        public:
            compilations_index (parse_stack_pointer s, transaction_pointer transaction,
                                names_pointer names);

            gsl::czstring name () const noexcept override;
            std::error_code key (std::string const & s) override;
            std::error_code end_object () override;

        private:
            transaction_pointer transaction_;
            names_pointer names_;
        };

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_COMPILATIONS_HPP
