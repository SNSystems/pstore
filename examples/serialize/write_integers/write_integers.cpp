//*                _ _         _       _                            *
//* __      ___ __(_) |_ ___  (_)_ __ | |_ ___  __ _  ___ _ __ ___  *
//* \ \ /\ / / '__| | __/ _ \ | | '_ \| __/ _ \/ _` |/ _ \ '__/ __| *
//*  \ V  V /| |  | | ||  __/ | | | | | ||  __/ (_| |  __/ |  \__ \ *
//*   \_/\_/ |_|  |_|\__\___| |_|_| |_|\__\___|\__, |\___|_|  |___/ *
//*                                            |___/                *
//===- examples/serialize/write_integers/write_integers.cpp ---------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

/// This example will write integers 10 and 20 (in that order) to a vector of
/// bytes managed by the vector_writer class. Finally, we write the accumulated
/// byte sequence (as a series of two-character hexadecimal digits) to cout.

#include <iostream>

#include "pstore/serialize/archive.hpp"
#include "pstore/serialize/types.hpp"

int main () {
    // Create the archive. This will be the sink for the data that we're about
    // to write.
    std::vector<std::uint8_t> bytes;
    pstore::serialize::archive::vector_writer writer{bytes};

    // Write two integers to the archive.
    pstore::serialize::write (writer, 10, 20);

    // Dump the raw contents of the archive.
    std::cout << "Wrote these bytes:\n" << writer << '\n';
}
