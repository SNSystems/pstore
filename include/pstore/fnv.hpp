//*   __             *
//*  / _|_ ____   __ *
//* | |_| '_ \ \ / / *
//* |  _| | | \ V /  *
//* |_| |_| |_|\_/   *
//*                  *
//===- include/pstore/fnv.hpp ---------------------------------------------===//
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
 * fnv - Fowler/Noll/Vo- hash code
 *
 * @(#) $Revision: 5.4 $
 * @(#) $Id: fnv.h,v 5.4 2009/07/30 22:49:13 chongo Exp $
 * @(#) $Source: /usr/local/src/cmd/fnv/RCS/fnv.h,v $
 *
 ***************************************************************************
 *                                                                         *
 * Fowler/Noll/Vo- hash                                                    *
 *                                                                         *
 * The basis of this hash algorithm was taken from an idea sent            *
 * as reviewer comments to the IEEE POSIX P1003.2 committee by:            *
 *                                                                         *
 *      Phong Vo (http://www.research.att.com/info/kpv/)                   *
 *      Glenn Fowler (http://www.research.att.com/~gsf/)                   *
 *                                                                         *
 * In a subsequent ballot round:                                           *
 *                                                                         *
 *      Landon Curt Noll (http://www.isthe.com/chongo/)                    *
 *                                                                         *
 * improved on their algorithm.  Some people tried this hash               *
 * and found that it worked rather well.  In an EMail message              *
 * to Landon, they named it the ``Fowler/Noll/Vo'' or FNV hash.            *
 *                                                                         *
 * FNV hashes are designed to be fast while maintaining a low              *
 * collision rate. The FNV speed allows one to quickly hash lots           *
 * of data while maintaining a reasonable collision rate.  See:            *
 *                                                                         *
 *      http://www.isthe.com/chongo/tech/comp/fnv/index.html               *
 *                                                                         *
 * for more details as well as other forms of the FNV hash.                *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 * Please do not copyright this code.  This code is in the public domain.  *
 *                                                                         *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, *
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO  *
 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR     *
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF  *
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR   *
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR  *
 * PERFORMANCE OF THIS SOFTWARE.                                           *
 *                                                                         *
 * By:                                                                     *
 *	chongo <Landon Curt Noll> /\oo/\                                       *
 *      http://www.isthe.com/chongo/                                       *
 *                                                                         *
 * Share and Enjoy!	:-)                                                    *
 ***************************************************************************
 */

#ifndef PSTORE_FNV_HPP
#define PSTORE_FNV_HPP

#include <cstdint>
#include <cstdlib>
#include <string>

#define PSTORE_FNV_VERSION "5.0.2" /* @(#) FNV Version */

namespace pstore {

    /// \brief 64 bit FNV-1 non-zero initial basis
    /// \note The FNV-1a initial basis is the same value as FNV-1 by definition.
    constexpr std::uint64_t fnv1_64_init = UINT64_C (0xcbf29ce484222325);
    constexpr std::uint64_t fnv1a_64_init = fnv1_64_init;


    /// \brief Perform a 64 bit Fowler/Noll/Vo FNV-1a hash on a buffer.
    /// \param buf  Start of buffer to hash
    /// \param len Length of buffer in octets
    /// \param hval  Previous hash value
    /// \returns 64 bit hash.
    /// \note  To use the recommended 64 bit FNV-1a hash, use fnv1a_64_init as the hval arg on the
    /// first call to either fnv_64a_buf() or fnv_64a_str().
    std::uint64_t fnv_64a_buf (void const * buf, std::size_t len,
                               std::uint64_t hval = fnv1a_64_init);

    /// \brief perform a 64 bit Fowler/Noll/Vo FNV-1a hash on a buffer
    /// \param str  Start of the NUL-terminated string to hash
    /// \param hval  Previous hash value
    /// \returns 64 bit hash.
    /// \note  To use the recommended 64 bit FNV-1a hash, use fnv1a_64_init as the hval arg on the
    /// first call to either fnv_64a_buf() or fnv_64a_str().
    std::uint64_t fnv_64a_str (char const * str, std::uint64_t hval = fnv1a_64_init);


    /// A simple function object wrapper for the fnv_64a_buf() function which is intended to
    /// be a compatible replacement for std::hash<>. It will hash the contents of any contiguous
    /// container (std::array<>, std::vector<>, and std::string<>.
    struct fnv_64a_hash {
        template <typename Container>
        std::uint64_t operator() (Container const & c) const {
            return ::pstore::fnv_64a_buf (c.data (), c.size ());
        }
    };

} // namespace pstore
#endif // PSTORE_FNV_HPP
// eof: include/pstore/fnv.hpp
