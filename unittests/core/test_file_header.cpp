//*   __ _ _        _                    _            *
//*  / _(_) | ___  | |__   ___  __ _  __| | ___ _ __  *
//* | |_| | |/ _ \ | '_ \ / _ \/ _` |/ _` |/ _ \ '__| *
//* |  _| | |  __/ | | | |  __/ (_| | (_| |  __/ |    *
//* |_| |_|_|\___| |_| |_|\___|\__,_|\__,_|\___|_|    *
//*                                                   *
//===- unittests/pstore/test_file_header.cpp ------------------------------===//
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
#include "pstore/core/file_header.hpp"

#include "gtest/gtest.h"


TEST (PstoreExtent, ComparisonOperators) {
    {
        auto const extent1 =
            make_extent (pstore::typed_address<std::uint8_t>::make (2), UINT64_C (4));
        auto const extent2 =
            make_extent (pstore::typed_address<std::uint8_t>::make (2), UINT64_C (4));

        EXPECT_TRUE (extent1 == extent2);
        EXPECT_TRUE (!(extent1 != extent2));
        EXPECT_TRUE (!(extent1 < extent2));
        EXPECT_TRUE (extent1 <= extent2);
        EXPECT_TRUE (!(extent1 > extent2));
        EXPECT_TRUE (extent1 >= extent2);
        EXPECT_TRUE (extent1 == extent1);
        EXPECT_TRUE (!(extent2 != extent1));
        EXPECT_TRUE (!(extent2 < extent1));
        EXPECT_TRUE (extent2 <= extent1);
        EXPECT_TRUE (!(extent2 > extent1));
        EXPECT_TRUE (extent2 >= extent1);
    }
    {
        auto const extent1 =
            make_extent (pstore::typed_address<std::uint8_t>::make (2), UINT64_C (4));
        auto const extent2 =
            make_extent (pstore::typed_address<std::uint8_t>::make (5), UINT64_C (4)); // bigger

        EXPECT_TRUE (extent1 != extent2);
        EXPECT_TRUE (extent2 != extent1);
        EXPECT_FALSE (extent1 == extent2);
        EXPECT_FALSE (extent2 == extent1);
        EXPECT_TRUE (extent1 < extent2);
        EXPECT_FALSE (extent2 < extent1);
        EXPECT_TRUE (extent1 <= extent2);
        EXPECT_TRUE (!(extent2 <= extent1));
        EXPECT_TRUE (extent2 > extent1);
        EXPECT_TRUE (!(extent1 > extent2));
        EXPECT_TRUE (extent2 >= extent1);
        EXPECT_TRUE (!(extent1 >= extent2));
    }
    {
        auto const extent1 =
            make_extent (pstore::typed_address<std::uint8_t>::make (2), UINT64_C (4));
        auto const extent2 =
            make_extent (pstore::typed_address<std::uint8_t>::make (2), UINT64_C (5)); // bigger

        EXPECT_TRUE (extent1 != extent2);
        EXPECT_TRUE (extent2 != extent1);
        EXPECT_FALSE (extent1 == extent2);
        EXPECT_FALSE (extent2 == extent1);
        EXPECT_TRUE (extent1 < extent2);
        EXPECT_FALSE (extent2 < extent1);
        EXPECT_TRUE (extent1 <= extent2);
        EXPECT_TRUE (!(extent2 <= extent1));
        EXPECT_TRUE (extent2 > extent1);
        EXPECT_TRUE (!(extent1 > extent2));
        EXPECT_TRUE (extent2 >= extent1);
        EXPECT_TRUE (!(extent1 >= extent2));
    }
}

// eof: unittests/pstore/test_file_header.cpp