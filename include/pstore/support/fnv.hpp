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
 *      chongo <Landon Curt Noll> /\oo/\                                   *
 *      http://www.isthe.com/chongo/                                       *
 *                                                                         *
 * Share and Enjoy!     :-)                                                *
 ***************************************************************************
 */

#ifndef PSTORE_SUPPORT_FNV_HPP
#define PSTORE_SUPPORT_FNV_HPP

#include <cstdint>
#include <cstdlib>

#include "pstore/support/gsl.hpp"

namespace pstore {

    constexpr char const * fnv_version = "5.0.2";

    /// \brief 64 bit FNV-1 non-zero initial basis
    /// \note The FNV-1a initial basis is the same value as FNV-1 by definition.
    constexpr std::uint64_t fnv1_64_init = UINT64_C (0xcbf29ce484222325);
    constexpr std::uint64_t fnv1a_64_init = fnv1_64_init;

    namespace fnv_details {

#ifdef NO_FNV_GCC_OPTIMIZATION
        /// 64 bit magic FNV-1a prime
        constexpr auto fnv_64_prime = UINT64_C (0x100000001b3);
#endif

        inline std::uint64_t append (std::uint8_t const v, std::uint64_t hval) noexcept {
            // xor the bottom with the current octet
            hval ^= static_cast<std::uint64_t> (v);

// multiply by the 64 bit FNV magic prime mod 2^64
#ifdef NO_FNV_GCC_OPTIMIZATION
            hval *= fnv_64_prime;
#else
            hval += (hval << 1U) + (hval << 4U) + (hval << 5U) + (hval << 7U) + (hval << 8U) +
                    (hval << 40U);
#endif
            return hval;
        }

    } // end namespace fnv_details


    /// \brief Perform a 64 bit Fowler/Noll/Vo FNV-1a hash on a buffer.
    /// \param buf  Buffer to hash
    /// \param hval  Previous hash value
    /// \returns 64 bit hash.
    /// \note  To use the recommended 64 bit FNV-1a hash, use fnv1a_64_init as the hval arg on the
    ///   first call to either fnv_64a_buf() or fnv_64a_str().
    template <typename ElementType, std::ptrdiff_t Extent>
    std::uint64_t fnv_64a_buf (gsl::span<ElementType, Extent> const buf,
                               std::uint64_t const hval = fnv1a_64_init) noexcept {
        // FNV-1a hash each octet of the buffer
        auto result = hval;
        for (auto const *it = reinterpret_cast<std::uint8_t const *> (buf.data ()),
                        *const end = it + buf.size_bytes ();
             it != end; ++it) {
            result = fnv_details::append (*it, result);
        }
        return result;
    }


    /// \brief perform a 64 bit Fowler/Noll/Vo FNV-1a hash on a buffer
    /// \param str  Start of the NUL-terminated string to hash
    /// \param hval  Previous hash value
    /// \returns 64 bit hash.
    /// \note  To use the recommended 64 bit FNV-1a hash, use fnv1a_64_init as the hval arg on the
    ///   first call to either fnv_64a_buf() or fnv_64a_str().
    std::uint64_t fnv_64a_str (gsl::czstring str, std::uint64_t hval = fnv1a_64_init) noexcept;


    /// A simple function object wrapper for the fnv_64a_buf() function which is intended to
    /// be a compatible replacement for std::hash<>. It will hash the contents of any contiguous
    /// container (std::array<>, std::vector<>, and std::string<>.
    struct fnv_64a_hash {
        template <typename Container>
        std::uint64_t operator() (Container const & c) const {
            return fnv_64a_buf (gsl::make_span (c));
        }
    };

} // namespace pstore
#endif // PSTORE_SUPPORT_FNV_HPP
