//*   __             *
//*  / _|_ ____   __ *
//* | |_| '_ \ \ / / *
//* |  _| | | \ V /  *
//* |_| |_| |_|\_/   *
//*                  *
//===- lib/pstore/fnv.cpp -------------------------------------------------===//
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
/*
 * hash_64 - 64 bit Fowler/Noll/Vo-0 FNV-1a hash code
 *
 * @(#) $Revision: 5.1 $
 * @(#) $Id: hash_64a.c,v 5.1 2009/06/30 09:01:38 chongo Exp $
 * @(#) $Source: /usr/local/src/cmd/fnv/RCS/hash_64a.c,v $
 *
 ***
 *
 * Fowler/Noll/Vo hash
 *
 * The basis of this hash algorithm was taken from an idea sent
 * as reviewer comments to the IEEE POSIX P1003.2 committee by:
 *
 *      Phong Vo (http://www.research.att.com/info/kpv/)
 *      Glenn Fowler (http://www.research.att.com/~gsf/)
 *
 * In a subsequent ballot round:
 *
 *      Landon Curt Noll (http://www.isthe.com/chongo/)
 *
 * improved on their algorithm.  Some people tried this hash
 * and found that it worked rather well.  In an EMail message
 * to Landon, they named it the ``Fowler/Noll/Vo'' or FNV hash.
 *
 * FNV hashes are designed to be fast while maintaining a low
 * collision rate. The FNV speed allows one to quickly hash lots
 * of data while maintaining a reasonable collision rate.  See:
 *
 *      http://www.isthe.com/chongo/tech/comp/fnv/index.html
 *
 * for more details as well as other forms of the FNV hash.
 *
 ***
 *
 * To use the recommended 64 bit FNV-1a hash, pass FNV1A_64_INIT as the
 * std::uint64_t hashval argument to fnv_64a_buf() or fnv_64a_str().
 *
 ***
 *
 * Please do not copyright this code.  This code is in the public domain.
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * By:
 *	chongo <Landon Curt Noll> /\oo/\
 *      http://www.isthe.com/chongo/
 *
 * Share and Enjoy!	:-)
 */

#include "pstore/fnv.hpp"

namespace pstore {

#ifdef NO_FNV_GCC_OPTIMIZATION
    /// 64 bit magic FNV-1a prime
    constexpr auto fnv_64_prime = UINT64_C (0x100000001b3);
#endif


    std::uint64_t fnv_64a_buf (void const * buf, std::size_t len, std::uint64_t hval) {
        // FNV-1a hash each octet of the buffer
        for (auto it = static_cast<uint8_t const *> (buf), end = it + len; it < end; ++it) {
            // xor the bottom with the current octet
            hval ^= static_cast<std::uint64_t> (*it);

// multiply by the 64 bit FNV magic prime mod 2^64
#ifdef NO_FNV_GCC_OPTIMIZATION
            hval *= fnv_64_prime;
#else
            hval +=
                (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40);
#endif
        }
        return hval;
    }


    std::uint64_t fnv_64a_str (char const * str, std::uint64_t hval) {
        // FNV-1a hash each octet of the string
        for (auto s = reinterpret_cast<uint8_t const *> (str); *s != '\0'; ++s) {
            // xor the bottom with the current octet
            hval ^= static_cast<std::uint64_t> (*s);

// multiply by the 64 bit FNV magic prime mod 2^64
#ifdef NO_FNV_GCC_OPTIMIZATION
            hval *= fnv_64_prime;
#else
            hval +=
                (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40);
#endif
        }
        return hval;
    }

} // namespace pstore
// eof: lib/pstore/fnv.cpp
