//*  _                     _                          _       *
//* | |_ ___   __   ____ _| |_   _  ___   _ __   __ _(_)_ __  *
//* | __/ _ \  \ \ / / _` | | | | |/ _ \ | '_ \ / _` | | '__| *
//* | || (_) |  \ V / (_| | | |_| |  __/ | |_) | (_| | | |    *
//*  \__\___/    \_/ \__,_|_|\__,_|\___| | .__/ \__,_|_|_|    *
//*                                      |_|                  *
//===- tools/write/to_value_pair.cpp --------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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
#include "to_value_pair.hpp"

#include <cassert>
#include <cstring>
#include <utility>

std::pair <std::string, std::string> to_value_pair (char const * option) {
    std::string key;
    std::string value;
    
    if (option != nullptr && option [0] != '\0') {
        if (char const * comma = std::strchr (option, ',')) {
            assert (comma >= option);
            key.assign (option, static_cast <std::string::size_type> (comma - option));
            value.assign (comma + 1);
        } else {
            key.assign (option);
        }
    }
    
    if (key.length () == 0 || value.length () == 0) {
        key.clear ();
        value.clear ();
    }
    return std::make_pair (key, value);
}
// eof: tools/write/to_value_pair.cpp
