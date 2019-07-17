//*  _               _                                   _           *
//* | |__  _ __ ___ | | _____ _ __   ___  ___ _ ____   _(_) ___ ___  *
//* | '_ \| '__/ _ \| |/ / _ \ '__| / __|/ _ \ '__\ \ / / |/ __/ _ \ *
//* | |_) | | | (_) |   <  __/ |    \__ \  __/ |   \ V /| | (_|  __/ *
//* |_.__/|_|  \___/|_|\_\___|_|    |___/\___|_|    \_/ |_|\___\___| *
//*                                                                  *
//===- tools/brokerd/service/broker_service.hpp ---------------------------===//
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

/// \file service_installer.hpp

#ifndef PSTORE_BROKER_SERVICE_HPP
#define PSTORE_BROKER_SERVICE_HPP

#include <atomic>
#include <thread>

#include "service_base.hpp"

class sample_service final : public service_base {
public:
    sample_service (TCHAR const * service_name, bool can_stop = true, bool can_shutdown = true,
                    bool can_pause_continue = false);
    ~sample_service () override;

protected:
    void start_handler (DWORD argc, TCHAR * argv[]) override;
    void stop_handler () override;

    void worker ();

private:
    std::atomic<bool> is_stopping_{false};
    std::thread thread_;
};

#endif // PSTORE_BROKER_SERVICE_HPP
