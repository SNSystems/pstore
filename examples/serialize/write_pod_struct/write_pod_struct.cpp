//*                _ _                         _       _                   _    *
//* __      ___ __(_) |_ ___   _ __   ___   __| |  ___| |_ _ __ _   _  ___| |_  *
//* \ \ /\ / / '__| | __/ _ \ | '_ \ / _ \ / _` | / __| __| '__| | | |/ __| __| *
//*  \ V  V /| |  | | ||  __/ | |_) | (_) | (_| | \__ \ |_| |  | |_| | (__| |_  *
//*   \_/\_/ |_|  |_|\__\___| | .__/ \___/ \__,_| |___/\__|_|   \__,_|\___|\__| *
//*                           |_|                                               *
//===- examples/serialize/write_pod_struct/write_pod_struct.cpp -----------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include <iostream>
#include <vector>
#include "pstore/serialize/archive.hpp"
#include "pstore/serialize/types.hpp"

struct pod_type {
    int a;
    int b;
};

int main () {
    using namespace pstore::serialize;
    std::vector<std::uint8_t> bytes;
    archive::vector_writer writer{bytes};
    pod_type pt{30, 40};
    write (writer, pt);
    std::cout << "Wrote these bytes:\n" << writer << '\n';
}
