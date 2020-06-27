//*                             _                    _ _    *
//*   _____  ___ __   ___  _ __| |_    ___ _ __ ___ (_) |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __|  / _ \ '_ ` _ \| | __| *
//* |  __/>  <| |_) | (_) | |  | |_  |  __/ | | | | | | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__|  \___|_| |_| |_|_|\__| *
//*           |_|                                           *
//===- unittests/exchange/test_export_emit.cpp ----------------------------===//
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
#include "pstore/exchange/export_emit.hpp"

#include <sstream>

#include "gtest/gtest.h"

TEST (ExportEmit, SimpleString) {
    {
        std::ostringstream os1;
        pstore::exchange::emit_string (os1, "");
        EXPECT_EQ (os1.str (), "\"\"");
    }
    {
        std::ostringstream os2;
        pstore::exchange::emit_string (os2, "hello");
        EXPECT_EQ (os2.str (), "\"hello\"");
    }
}

TEST (ExportEmit, EscapeQuotes) {
    std::ostringstream os;
    pstore::exchange::emit_string (os, "a \" b");
    EXPECT_EQ (os.str (), "\"a \\\" b\"");
}

TEST (ExportEmit, EscapeBackslash) {
    std::ostringstream os;
    pstore::exchange::emit_string (os, "\\");
    EXPECT_EQ (os.str (), "\"\\\\\"");
}
