//*                _ _             *
//* __      ___ __(_) |_ ___ _ __  *
//* \ \ /\ / / '__| | __/ _ \ '__| *
//*  \ V  V /| |  | | ||  __/ |    *
//*   \_/\_/ |_|  |_|\__\___|_|    *
//*                                *
//===- lib/brokerface/writer_posix.cpp ------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file writer_posix.cpp
/// \brief Implements the parts of the class which enables a client to send messages to the broker
/// which are POSIX-specific.

#include "pstore/brokerface/writer.hpp"

#ifndef _WIN32

#    include <unistd.h>
#    include "pstore/brokerface/message_type.hpp"
#    include "pstore/support/error.hpp"

namespace pstore {
    namespace brokerface {

        bool writer::write_impl (message_type const & msg) {
            bool const ok = ::write (fd_.native_handle (), &msg, sizeof (msg)) == sizeof (msg);
            if (!ok) {
                int const err = errno;
                if (err != EAGAIN && err != EWOULDBLOCK && err != EPIPE) {
                    raise (errno_erc{err}, "write to broker pipe");
                }
            }
            return ok;
        }

    } // end namespace brokerface
} // end namespace pstore

#endif // _WIN32
