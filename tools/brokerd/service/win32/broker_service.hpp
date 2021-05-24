//===- tools/brokerd/service/win32/broker_service.hpp -----*- mode: C++ -*-===//
//*  _               _                                   _           *
//* | |__  _ __ ___ | | _____ _ __   ___  ___ _ ____   _(_) ___ ___  *
//* | '_ \| '__/ _ \| |/ / _ \ '__| / __|/ _ \ '__\ \ / / |/ __/ _ \ *
//* | |_) | | | (_) |   <  __/ |    \__ \  __/ |   \ V /| | (_|  __/ *
//* |_.__/|_|  \___/|_|\_\___|_|    |___/\___|_|    \_/ |_|\___\___| *
//*                                                                  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/// \file service_installer.hpp

#ifndef PSTORE_BROKER_SERVICE_HPP
#define PSTORE_BROKER_SERVICE_HPP

#include <atomic>
#include <future>

#include "service_base.hpp"
#include "../../switches.hpp"
#include "pstore/support/maybe.hpp"

// Platform includes
#ifndef _WIN32
#    include <signal.h>
#endif

class broker_service final : public service_base {
public:
    broker_service (TCHAR const * service_name, bool can_stop = true, bool can_shutdown = true,
                    bool can_pause_continue = false);
    ~broker_service () override;

protected:
    void start_handler (DWORD argc, TCHAR * argv[]) override;
    void stop_handler () override;

    void worker (switches opt);

private:
    std::thread worker_thread_;
};

#endif // PSTORE_BROKER_SERVICE_HPP
