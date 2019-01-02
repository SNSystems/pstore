//*      _        _              *
//*  ___| |_ __ _| |_ _   _ ___  *
//* / __| __/ _` | __| | | / __| *
//* \__ \ || (_| | |_| |_| \__ \ *
//* |___/\__\__,_|\__|\__,_|___/ *
//*                              *
//===- include/pstore/vacuum/status.hpp -----------------------------------===//
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

// Copyright (c) 2015-2016 by Sony Interactive Entertainment, Inc.
/// \file status.hpp

#ifndef VACUUM_STATUS_HPP
#define VACUUM_STATUS_HPP (1)

#include <atomic>
#include <chrono>

namespace vacuum {

    struct status {
        std::atomic<bool> modified{false};
        std::atomic<bool> done{false};
        std::atomic<bool> watch_running{false};
    };

    auto constexpr initial_delay = std::chrono::seconds (10);
    auto constexpr watch_interval = std::chrono::milliseconds (500);

} // namespace vacuum
#endif // VACUUM_STATUS_HPP
