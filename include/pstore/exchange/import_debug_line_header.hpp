//*  _                            _         _      _                  *
//* (_)_ __ ___  _ __   ___  _ __| |_    __| | ___| |__  _   _  __ _  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __|  / _` |/ _ \ '_ \| | | |/ _` | *
//* | | | | | | | |_) | (_) | |  | |_  | (_| |  __/ |_) | |_| | (_| | *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \__,_|\___|_.__/ \__,_|\__, | *
//*             |_|                                            |___/  *
//*  _ _              _                    _            *
//* | (_)_ __   ___  | |__   ___  __ _  __| | ___ _ __  *
//* | | | '_ \ / _ \ | '_ \ / _ \/ _` |/ _` |/ _ \ '__| *
//* | | | | | |  __/ | | | |  __/ (_| | (_| |  __/ |    *
//* |_|_|_| |_|\___| |_| |_|\___|\__,_|\__,_|\___|_|    *
//*                                                     *
//===- include/pstore/exchange/import_debug_line_header.hpp ---------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_EXCHANGE_IMPORT_DEBUG_LINE_HEADER_HPP
#define PSTORE_EXCHANGE_IMPORT_DEBUG_LINE_HEADER_HPP

#include "pstore/core/hamt_map.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/exchange/import_error.hpp"
#include "pstore/exchange/import_rule.hpp"
#include "pstore/support/base64.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            class debug_line_index final : public rule {
            public:
                debug_line_index (not_null<context *> ctxt,
                                  not_null<transaction_base *> transaction);
                debug_line_index (debug_line_index const &) = delete;
                debug_line_index (debug_line_index &&) noexcept = delete;

                ~debug_line_index () noexcept override = default;

                debug_line_index & operator= (debug_line_index const &) = delete;
                debug_line_index & operator= (debug_line_index &&) noexcept = delete;

                gsl::czstring name () const noexcept override;

                std::error_code string_value (std::string const & s) override;
                std::error_code key (std::string const & s) override;
                std::error_code end_object () override;

            private:
                std::shared_ptr<index::debug_line_header_index> index_;
                index::digest digest_;

                not_null<transaction_base *> transaction_;
            };

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_DEBUG_LINE_HEADER_HPP
