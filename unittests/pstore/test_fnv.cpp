/*
 * test_fnv - FNV test suite
 *
 * @(#) $Revision: 5.3 $
 * @(#) $Id: test_fnv.c,v 5.3 2009/06/30 11:50:41 chongo Exp $
 * @(#) $Source: /usr/local/src/cmd/fnv/RCS/test_fnv.c,v $
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
#include <gtest/gtest.h>

// FNVTEST macro does not include trailing NUL byte in the test vector
#define FNVTEST(x)                                                                                 \
    { x, sizeof (x) - 1 }

// FNVTEST0 macro includes the trailing NUL byte in the test vector
#define FNVTEST0(x)                                                                                \
    { x, sizeof (x) }

// REPEAT500 - repeat a string 500 times
#define R500(x) R100 (x) R100 (x) R100 (x) R100 (x) R100 (x)
#define R100(x) R10 (x) R10 (x) R10 (x) R10 (x) R10 (x) R10 (x) R10 (x) R10 (x) R10 (x) R10 (x)
#define R10(x) x x x x x x x x x x

namespace {
    // these test vectors are used as part of the FNV test suite
    struct test_vector {
        void const * buf; // start of test vector buffer
        std::size_t len;  // length of test vector
    };
    struct fnv1a_64_test_vector {
        test_vector const * test; // test vector buffer to hash
        std::uint64_t fnv1a_64;   // expected FNV-1a 64 bit hash value
    };

    /*
     * FNV test vectors
     *
     * NOTE: A nullptr pointer marks beyond the end of the test vectors.
     *
     * NOTE: The order of the fnv_test_str[] test vectors is 1-to-1 with:
     *
     *	struct fnv0_32_test_vector fnv0_32_vector[];
     *	struct fnv1_32_test_vector fnv1_32_vector[];
     *	struct fnv1a_32_test_vector fnv1a_32_vector[];
     *	struct fnv0_64_test_vector fnv0_64_vector[];
     *	struct fnv1_64_test_vector fnv1_64_vector[];
     *	struct fnv1a_64_test_vector fnv1a_64_vector[];
     *
     * IMPORTANT NOTE:
     *
     *	If you change the fnv_test_str[] array, you need
     *	to also change ALL of the above fnv*_vector arrays!!!
     *
     *	To rebuild, try:
     *
     *		make vector.c
     *
     *	and then fold the results into the source file.
     *	Of course, you better make sure that the vaules
     *	produced by the above command are valid, otherwise
     * 	you will be testing against invalid vectors!
     */
    test_vector const fnv_test_str[] = {
        FNVTEST (""),
        FNVTEST ("a"),
        FNVTEST ("b"),
        FNVTEST ("c"),
        FNVTEST ("d"),
        FNVTEST ("e"),
        FNVTEST ("f"),
        FNVTEST ("fo"),
        FNVTEST ("foo"),
        FNVTEST ("foob"),
        FNVTEST ("fooba"),
        FNVTEST ("foobar"),
        FNVTEST0 (""),
        FNVTEST0 ("a"),
        FNVTEST0 ("b"),
        FNVTEST0 ("c"),
        FNVTEST0 ("d"),
        FNVTEST0 ("e"),
        FNVTEST0 ("f"),
        FNVTEST0 ("fo"),
        FNVTEST0 ("foo"),
        FNVTEST0 ("foob"),
        FNVTEST0 ("fooba"),
        FNVTEST0 ("foobar"),
        FNVTEST ("ch"),
        FNVTEST ("cho"),
        FNVTEST ("chon"),
        FNVTEST ("chong"),
        FNVTEST ("chongo"),
        FNVTEST ("chongo "),
        FNVTEST ("chongo w"),
        FNVTEST ("chongo wa"),
        FNVTEST ("chongo was"),
        FNVTEST ("chongo was "),
        FNVTEST ("chongo was h"),
        FNVTEST ("chongo was he"),
        FNVTEST ("chongo was her"),
        FNVTEST ("chongo was here"),
        FNVTEST ("chongo was here!"),
        FNVTEST ("chongo was here!\n"),
        FNVTEST0 ("ch"),
        FNVTEST0 ("cho"),
        FNVTEST0 ("chon"),
        FNVTEST0 ("chong"),
        FNVTEST0 ("chongo"),
        FNVTEST0 ("chongo "),
        FNVTEST0 ("chongo w"),
        FNVTEST0 ("chongo wa"),
        FNVTEST0 ("chongo was"),
        FNVTEST0 ("chongo was "),
        FNVTEST0 ("chongo was h"),
        FNVTEST0 ("chongo was he"),
        FNVTEST0 ("chongo was her"),
        FNVTEST0 ("chongo was here"),
        FNVTEST0 ("chongo was here!"),
        FNVTEST0 ("chongo was here!\n"),
        FNVTEST ("cu"),
        FNVTEST ("cur"),
        FNVTEST ("curd"),
        FNVTEST ("curds"),
        FNVTEST ("curds "),
        FNVTEST ("curds a"),
        FNVTEST ("curds an"),
        FNVTEST ("curds and"),
        FNVTEST ("curds and "),
        FNVTEST ("curds and w"),
        FNVTEST ("curds and wh"),
        FNVTEST ("curds and whe"),
        FNVTEST ("curds and whey"),
        FNVTEST ("curds and whey\n"),
        FNVTEST0 ("cu"),
        FNVTEST0 ("cur"),
        FNVTEST0 ("curd"),
        FNVTEST0 ("curds"),
        FNVTEST0 ("curds "),
        FNVTEST0 ("curds a"),
        FNVTEST0 ("curds an"),
        FNVTEST0 ("curds and"),
        FNVTEST0 ("curds and "),
        FNVTEST0 ("curds and w"),
        FNVTEST0 ("curds and wh"),
        FNVTEST0 ("curds and whe"),
        FNVTEST0 ("curds and whey"),
        FNVTEST0 ("curds and whey\n"),
        FNVTEST ("hi"),
        FNVTEST0 ("hi"),
        FNVTEST ("hello"),
        FNVTEST0 ("hello"),
        FNVTEST ("\xff\x00\x00\x01"),
        FNVTEST ("\x01\x00\x00\xff"),
        FNVTEST ("\xff\x00\x00\x02"),
        FNVTEST ("\x02\x00\x00\xff"),
        FNVTEST ("\xff\x00\x00\x03"),
        FNVTEST ("\x03\x00\x00\xff"),
        FNVTEST ("\xff\x00\x00\x04"),
        FNVTEST ("\x04\x00\x00\xff"),
        FNVTEST ("\x40\x51\x4e\x44"),
        FNVTEST ("\x44\x4e\x51\x40"),
        FNVTEST ("\x40\x51\x4e\x4a"),
        FNVTEST ("\x4a\x4e\x51\x40"),
        FNVTEST ("\x40\x51\x4e\x54"),
        FNVTEST ("\x54\x4e\x51\x40"),
        FNVTEST ("127.0.0.1"),
        FNVTEST0 ("127.0.0.1"),
        FNVTEST ("127.0.0.2"),
        FNVTEST0 ("127.0.0.2"),
        FNVTEST ("127.0.0.3"),
        FNVTEST0 ("127.0.0.3"),
        FNVTEST ("64.81.78.68"),
        FNVTEST0 ("64.81.78.68"),
        FNVTEST ("64.81.78.74"),
        FNVTEST0 ("64.81.78.74"),
        FNVTEST ("64.81.78.84"),
        FNVTEST0 ("64.81.78.84"),
        FNVTEST ("feedface"),
        FNVTEST0 ("feedface"),
        FNVTEST ("feedfacedaffdeed"),
        FNVTEST0 ("feedfacedaffdeed"),
        FNVTEST ("feedfacedeadbeef"),
        FNVTEST0 ("feedfacedeadbeef"),
        FNVTEST ("line 1\nline 2\nline 3"),
        FNVTEST ("chongo <Landon Curt Noll> /\\../\\"),
        FNVTEST0 ("chongo <Landon Curt Noll> /\\../\\"),
        FNVTEST ("chongo (Landon Curt Noll) /\\../\\"),
        FNVTEST0 ("chongo (Landon Curt Noll) /\\../\\"),
        FNVTEST ("http://antwrp.gsfc.nasa.gov/apod/astropix.html"),
        FNVTEST ("http://en.wikipedia.org/wiki/Fowler_Noll_Vo_hash"),
        FNVTEST ("http://epod.usra.edu/"),
        FNVTEST ("http://exoplanet.eu/"),
        FNVTEST ("http://hvo.wr.usgs.gov/cam3/"),
        FNVTEST ("http://hvo.wr.usgs.gov/cams/HMcam/"),
        FNVTEST ("http://hvo.wr.usgs.gov/kilauea/update/deformation.html"),
        FNVTEST ("http://hvo.wr.usgs.gov/kilauea/update/images.html"),
        FNVTEST ("http://hvo.wr.usgs.gov/kilauea/update/maps.html"),
        FNVTEST ("http://hvo.wr.usgs.gov/volcanowatch/current_issue.html"),
        FNVTEST ("http://neo.jpl.nasa.gov/risk/"),
        FNVTEST ("http://norvig.com/21-days.html"),
        FNVTEST ("http://primes.utm.edu/curios/home.php"),
        FNVTEST ("http://slashdot.org/"),
        FNVTEST ("http://tux.wr.usgs.gov/Maps/155.25-19.5.html"),
        FNVTEST ("http://volcano.wr.usgs.gov/kilaueastatus.php"),
        FNVTEST ("http://www.avo.alaska.edu/activity/Redoubt.php"),
        FNVTEST ("http://www.dilbert.com/fast/"),
        FNVTEST ("http://www.fourmilab.ch/gravitation/orbits/"),
        FNVTEST ("http://www.fpoa.net/"),
        FNVTEST ("http://www.ioccc.org/index.html"),
        FNVTEST ("http://www.isthe.com/cgi-bin/number.cgi"),
        FNVTEST ("http://www.isthe.com/chongo/bio.html"),
        FNVTEST ("http://www.isthe.com/chongo/index.html"),
        FNVTEST ("http://www.isthe.com/chongo/src/calc/lucas-calc"),
        FNVTEST ("http://www.isthe.com/chongo/tech/astro/venus2004.html"),
        FNVTEST ("http://www.isthe.com/chongo/tech/astro/vita.html"),
        FNVTEST ("http://www.isthe.com/chongo/tech/comp/c/expert.html"),
        FNVTEST ("http://www.isthe.com/chongo/tech/comp/calc/index.html"),
        FNVTEST ("http://www.isthe.com/chongo/tech/comp/fnv/index.html"),
        FNVTEST ("http://www.isthe.com/chongo/tech/math/number/howhigh.html"),
        FNVTEST ("http://www.isthe.com/chongo/tech/math/number/number.html"),
        FNVTEST ("http://www.isthe.com/chongo/tech/math/prime/mersenne.html"),
        FNVTEST ("http://www.isthe.com/chongo/tech/math/prime/mersenne.html#largest"),
        FNVTEST ("http://www.lavarnd.org/cgi-bin/corpspeak.cgi"),
        FNVTEST ("http://www.lavarnd.org/cgi-bin/haiku.cgi"),
        FNVTEST ("http://www.lavarnd.org/cgi-bin/rand-none.cgi"),
        FNVTEST ("http://www.lavarnd.org/cgi-bin/randdist.cgi"),
        FNVTEST ("http://www.lavarnd.org/index.html"),
        FNVTEST ("http://www.lavarnd.org/what/nist-test.html"),
        FNVTEST ("http://www.macosxhints.com/"),
        FNVTEST ("http://www.mellis.com/"),
        FNVTEST ("http://www.nature.nps.gov/air/webcams/parks/havoso2alert/havoalert.cfm"),
        FNVTEST ("http://www.nature.nps.gov/air/webcams/parks/havoso2alert/timelines_24.cfm"),
        FNVTEST ("http://www.paulnoll.com/"),
        FNVTEST ("http://www.pepysdiary.com/"),
        FNVTEST ("http://www.sciencenews.org/index/home/activity/view"),
        FNVTEST ("http://www.skyandtelescope.com/"),
        FNVTEST ("http://www.sput.nl/~rob/sirius.html"),
        FNVTEST ("http://www.systemexperts.com/"),
        FNVTEST ("http://www.tq-international.com/phpBB3/index.php"),
        FNVTEST ("http://www.travelquesttours.com/index.htm"),
        FNVTEST ("http://www.wunderground.com/global/stations/89606.html"),
        FNVTEST (R10 ("21701")),
        FNVTEST (R10 ("M21701")),
        FNVTEST (R10 ("2^21701-1")),
        FNVTEST (R10 ("\x54\xc5")),
        FNVTEST (R10 ("\xc5\x54")),
        FNVTEST (R10 ("23209")),
        FNVTEST (R10 ("M23209")),
        FNVTEST (R10 ("2^23209-1")),
        FNVTEST (R10 ("\x5a\xa9")),
        FNVTEST (R10 ("\xa9\x5a")),
        FNVTEST (R10 ("391581216093")),
        FNVTEST (R10 ("391581*2^216093-1")),
        FNVTEST (R10 ("\x05\xf9\x9d\x03\x4c\x81")),
        FNVTEST (R10 ("FEDCBA9876543210")),
        FNVTEST (R10 ("\xfe\xdc\xba\x98\x76\x54\x32\x10")),
        FNVTEST (R10 ("EFCDAB8967452301")),
        FNVTEST (R10 ("\xef\xcd\xab\x89\x67\x45\x23\x01")),
        FNVTEST (R10 ("0123456789ABCDEF")),
        FNVTEST (R10 ("\x01\x23\x45\x67\x89\xab\xcd\xef")),
        FNVTEST (R10 ("1032547698BADCFE")),
        FNVTEST (R10 ("\x10\x32\x54\x76\x98\xba\xdc\xfe")),
        FNVTEST (R500 ("\x00")),
        FNVTEST (R500 ("\x07")),
        FNVTEST (R500 ("~")),
        FNVTEST (R500 ("\x7f")),
        {nullptr, 0} /* MUST BE LAST */
    };


    /*
     * insert the contents of vector.c below
     *
     *	make vector.c
     *	:r vector.c
     */
    /* start of output generated by make vector.c */


    /* FNV-1a 64 bit test vectors */
    fnv1a_64_test_vector const fnv1a_64_vector[] = {
        {&fnv_test_str[0], UINT64_C (0xcbf29ce484222325)},
        {&fnv_test_str[1], UINT64_C (0xaf63dc4c8601ec8c)},
        {&fnv_test_str[2], UINT64_C (0xaf63df4c8601f1a5)},
        {&fnv_test_str[3], UINT64_C (0xaf63de4c8601eff2)},
        {&fnv_test_str[4], UINT64_C (0xaf63d94c8601e773)},
        {&fnv_test_str[5], UINT64_C (0xaf63d84c8601e5c0)},
        {&fnv_test_str[6], UINT64_C (0xaf63db4c8601ead9)},
        {&fnv_test_str[7], UINT64_C (0x08985907b541d342)},
        {&fnv_test_str[8], UINT64_C (0xdcb27518fed9d577)},
        {&fnv_test_str[9], UINT64_C (0xdd120e790c2512af)},
        {&fnv_test_str[10], UINT64_C (0xcac165afa2fef40a)},
        {&fnv_test_str[11], UINT64_C (0x85944171f73967e8)},
        {&fnv_test_str[12], UINT64_C (0xaf63bd4c8601b7df)},
        {&fnv_test_str[13], UINT64_C (0x089be207b544f1e4)},
        {&fnv_test_str[14], UINT64_C (0x08a61407b54d9b5f)},
        {&fnv_test_str[15], UINT64_C (0x08a2ae07b54ab836)},
        {&fnv_test_str[16], UINT64_C (0x0891b007b53c4869)},
        {&fnv_test_str[17], UINT64_C (0x088e4a07b5396540)},
        {&fnv_test_str[18], UINT64_C (0x08987c07b5420ebb)},
        {&fnv_test_str[19], UINT64_C (0xdcb28a18fed9f926)},
        {&fnv_test_str[20], UINT64_C (0xdd1270790c25b935)},
        {&fnv_test_str[21], UINT64_C (0xcac146afa2febf5d)},
        {&fnv_test_str[22], UINT64_C (0x8593d371f738acfe)},
        {&fnv_test_str[23], UINT64_C (0x34531ca7168b8f38)},
        {&fnv_test_str[24], UINT64_C (0x08a25607b54a22ae)},
        {&fnv_test_str[25], UINT64_C (0xf5faf0190cf90df3)},
        {&fnv_test_str[26], UINT64_C (0xf27397910b3221c7)},
        {&fnv_test_str[27], UINT64_C (0x2c8c2b76062f22e0)},
        {&fnv_test_str[28], UINT64_C (0xe150688c8217b8fd)},
        {&fnv_test_str[29], UINT64_C (0xf35a83c10e4f1f87)},
        {&fnv_test_str[30], UINT64_C (0xd1edd10b507344d0)},
        {&fnv_test_str[31], UINT64_C (0x2a5ee739b3ddb8c3)},
        {&fnv_test_str[32], UINT64_C (0xdcfb970ca1c0d310)},
        {&fnv_test_str[33], UINT64_C (0x4054da76daa6da90)},
        {&fnv_test_str[34], UINT64_C (0xf70a2ff589861368)},
        {&fnv_test_str[35], UINT64_C (0x4c628b38aed25f17)},
        {&fnv_test_str[36], UINT64_C (0x9dd1f6510f78189f)},
        {&fnv_test_str[37], UINT64_C (0xa3de85bd491270ce)},
        {&fnv_test_str[38], UINT64_C (0x858e2fa32a55e61d)},
        {&fnv_test_str[39], UINT64_C (0x46810940eff5f915)},
        {&fnv_test_str[40], UINT64_C (0xf5fadd190cf8edaa)},
        {&fnv_test_str[41], UINT64_C (0xf273ed910b32b3e9)},
        {&fnv_test_str[42], UINT64_C (0x2c8c5276062f6525)},
        {&fnv_test_str[43], UINT64_C (0xe150b98c821842a0)},
        {&fnv_test_str[44], UINT64_C (0xf35aa3c10e4f55e7)},
        {&fnv_test_str[45], UINT64_C (0xd1ed680b50729265)},
        {&fnv_test_str[46], UINT64_C (0x2a5f0639b3dded70)},
        {&fnv_test_str[47], UINT64_C (0xdcfbaa0ca1c0f359)},
        {&fnv_test_str[48], UINT64_C (0x4054ba76daa6a430)},
        {&fnv_test_str[49], UINT64_C (0xf709c7f5898562b0)},
        {&fnv_test_str[50], UINT64_C (0x4c62e638aed2f9b8)},
        {&fnv_test_str[51], UINT64_C (0x9dd1a8510f779415)},
        {&fnv_test_str[52], UINT64_C (0xa3de2abd4911d62d)},
        {&fnv_test_str[53], UINT64_C (0x858e0ea32a55ae0a)},
        {&fnv_test_str[54], UINT64_C (0x46810f40eff60347)},
        {&fnv_test_str[55], UINT64_C (0xc33bce57bef63eaf)},
        {&fnv_test_str[56], UINT64_C (0x08a24307b54a0265)},
        {&fnv_test_str[57], UINT64_C (0xf5b9fd190cc18d15)},
        {&fnv_test_str[58], UINT64_C (0x4c968290ace35703)},
        {&fnv_test_str[59], UINT64_C (0x07174bd5c64d9350)},
        {&fnv_test_str[60], UINT64_C (0x5a294c3ff5d18750)},
        {&fnv_test_str[61], UINT64_C (0x05b3c1aeb308b843)},
        {&fnv_test_str[62], UINT64_C (0xb92a48da37d0f477)},
        {&fnv_test_str[63], UINT64_C (0x73cdddccd80ebc49)},
        {&fnv_test_str[64], UINT64_C (0xd58c4c13210a266b)},
        {&fnv_test_str[65], UINT64_C (0xe78b6081243ec194)},
        {&fnv_test_str[66], UINT64_C (0xb096f77096a39f34)},
        {&fnv_test_str[67], UINT64_C (0xb425c54ff807b6a3)},
        {&fnv_test_str[68], UINT64_C (0x23e520e2751bb46e)},
        {&fnv_test_str[69], UINT64_C (0x1a0b44ccfe1385ec)},
        {&fnv_test_str[70], UINT64_C (0xf5ba4b190cc2119f)},
        {&fnv_test_str[71], UINT64_C (0x4c962690ace2baaf)},
        {&fnv_test_str[72], UINT64_C (0x0716ded5c64cda19)},
        {&fnv_test_str[73], UINT64_C (0x5a292c3ff5d150f0)},
        {&fnv_test_str[74], UINT64_C (0x05b3e0aeb308ecf0)},
        {&fnv_test_str[75], UINT64_C (0xb92a5eda37d119d9)},
        {&fnv_test_str[76], UINT64_C (0x73ce41ccd80f6635)},
        {&fnv_test_str[77], UINT64_C (0xd58c2c132109f00b)},
        {&fnv_test_str[78], UINT64_C (0xe78baf81243f47d1)},
        {&fnv_test_str[79], UINT64_C (0xb0968f7096a2ee7c)},
        {&fnv_test_str[80], UINT64_C (0xb425a84ff807855c)},
        {&fnv_test_str[81], UINT64_C (0x23e4e9e2751b56f9)},
        {&fnv_test_str[82], UINT64_C (0x1a0b4eccfe1396ea)},
        {&fnv_test_str[83], UINT64_C (0x54abd453bb2c9004)},
        {&fnv_test_str[84], UINT64_C (0x08ba5f07b55ec3da)},
        {&fnv_test_str[85], UINT64_C (0x337354193006cb6e)},
        {&fnv_test_str[86], UINT64_C (0xa430d84680aabd0b)},
        {&fnv_test_str[87], UINT64_C (0xa9bc8acca21f39b1)},
        {&fnv_test_str[88], UINT64_C (0x6961196491cc682d)},
        {&fnv_test_str[89], UINT64_C (0xad2bb1774799dfe9)},
        {&fnv_test_str[90], UINT64_C (0x6961166491cc6314)},
        {&fnv_test_str[91], UINT64_C (0x8d1bb3904a3b1236)},
        {&fnv_test_str[92], UINT64_C (0x6961176491cc64c7)},
        {&fnv_test_str[93], UINT64_C (0xed205d87f40434c7)},
        {&fnv_test_str[94], UINT64_C (0x6961146491cc5fae)},
        {&fnv_test_str[95], UINT64_C (0xcd3baf5e44f8ad9c)},
        {&fnv_test_str[96], UINT64_C (0xe3b36596127cd6d8)},
        {&fnv_test_str[97], UINT64_C (0xf77f1072c8e8a646)},
        {&fnv_test_str[98], UINT64_C (0xe3b36396127cd372)},
        {&fnv_test_str[99], UINT64_C (0x6067dce9932ad458)},
        {&fnv_test_str[100], UINT64_C (0xe3b37596127cf208)},
        {&fnv_test_str[101], UINT64_C (0x4b7b10fa9fe83936)},
        {&fnv_test_str[102], UINT64_C (0xaabafe7104d914be)},
        {&fnv_test_str[103], UINT64_C (0xf4d3180b3cde3eda)},
        {&fnv_test_str[104], UINT64_C (0xaabafd7104d9130b)},
        {&fnv_test_str[105], UINT64_C (0xf4cfb20b3cdb5bb1)},
        {&fnv_test_str[106], UINT64_C (0xaabafc7104d91158)},
        {&fnv_test_str[107], UINT64_C (0xf4cc4c0b3cd87888)},
        {&fnv_test_str[108], UINT64_C (0xe729bac5d2a8d3a7)},
        {&fnv_test_str[109], UINT64_C (0x74bc0524f4dfa4c5)},
        {&fnv_test_str[110], UINT64_C (0xe72630c5d2a5b352)},
        {&fnv_test_str[111], UINT64_C (0x6b983224ef8fb456)},
        {&fnv_test_str[112], UINT64_C (0xe73042c5d2ae266d)},
        {&fnv_test_str[113], UINT64_C (0x8527e324fdeb4b37)},
        {&fnv_test_str[114], UINT64_C (0x0a83c86fee952abc)},
        {&fnv_test_str[115], UINT64_C (0x7318523267779d74)},
        {&fnv_test_str[116], UINT64_C (0x3e66d3d56b8caca1)},
        {&fnv_test_str[117], UINT64_C (0x956694a5c0095593)},
        {&fnv_test_str[118], UINT64_C (0xcac54572bb1a6fc8)},
        {&fnv_test_str[119], UINT64_C (0xa7a4c9f3edebf0d8)},
        {&fnv_test_str[120], UINT64_C (0x7829851fac17b143)},
        {&fnv_test_str[121], UINT64_C (0x2c8f4c9af81bcf06)},
        {&fnv_test_str[122], UINT64_C (0xd34e31539740c732)},
        {&fnv_test_str[123], UINT64_C (0x3605a2ac253d2db1)},
        {&fnv_test_str[124], UINT64_C (0x08c11b8346f4a3c3)},
        {&fnv_test_str[125], UINT64_C (0x6be396289ce8a6da)},
        {&fnv_test_str[126], UINT64_C (0xd9b957fb7fe794c5)},
        {&fnv_test_str[127], UINT64_C (0x05be33da04560a93)},
        {&fnv_test_str[128], UINT64_C (0x0957f1577ba9747c)},
        {&fnv_test_str[129], UINT64_C (0xda2cc3acc24fba57)},
        {&fnv_test_str[130], UINT64_C (0x74136f185b29e7f0)},
        {&fnv_test_str[131], UINT64_C (0xb2f2b4590edb93b2)},
        {&fnv_test_str[132], UINT64_C (0xb3608fce8b86ae04)},
        {&fnv_test_str[133], UINT64_C (0x4a3a865079359063)},
        {&fnv_test_str[134], UINT64_C (0x5b3a7ef496880a50)},
        {&fnv_test_str[135], UINT64_C (0x48fae3163854c23b)},
        {&fnv_test_str[136], UINT64_C (0x07aaa640476e0b9a)},
        {&fnv_test_str[137], UINT64_C (0x2f653656383a687d)},
        {&fnv_test_str[138], UINT64_C (0xa1031f8e7599d79c)},
        {&fnv_test_str[139], UINT64_C (0xa31908178ff92477)},
        {&fnv_test_str[140], UINT64_C (0x097edf3c14c3fb83)},
        {&fnv_test_str[141], UINT64_C (0xb51ca83feaa0971b)},
        {&fnv_test_str[142], UINT64_C (0xdd3c0d96d784f2e9)},
        {&fnv_test_str[143], UINT64_C (0x86cd26a9ea767d78)},
        {&fnv_test_str[144], UINT64_C (0xe6b215ff54a30c18)},
        {&fnv_test_str[145], UINT64_C (0xec5b06a1c5531093)},
        {&fnv_test_str[146], UINT64_C (0x45665a929f9ec5e5)},
        {&fnv_test_str[147], UINT64_C (0x8c7609b4a9f10907)},
        {&fnv_test_str[148], UINT64_C (0x89aac3a491f0d729)},
        {&fnv_test_str[149], UINT64_C (0x32ce6b26e0f4a403)},
        {&fnv_test_str[150], UINT64_C (0x614ab44e02b53e01)},
        {&fnv_test_str[151], UINT64_C (0xfa6472eb6eef3290)},
        {&fnv_test_str[152], UINT64_C (0x9e5d75eb1948eb6a)},
        {&fnv_test_str[153], UINT64_C (0xb6d12ad4a8671852)},
        {&fnv_test_str[154], UINT64_C (0x88826f56eba07af1)},
        {&fnv_test_str[155], UINT64_C (0x44535bf2645bc0fd)},
        {&fnv_test_str[156], UINT64_C (0x169388ffc21e3728)},
        {&fnv_test_str[157], UINT64_C (0xf68aac9e396d8224)},
        {&fnv_test_str[158], UINT64_C (0x8e87d7e7472b3883)},
        {&fnv_test_str[159], UINT64_C (0x295c26caa8b423de)},
        {&fnv_test_str[160], UINT64_C (0x322c814292e72176)},
        {&fnv_test_str[161], UINT64_C (0x8a06550eb8af7268)},
        {&fnv_test_str[162], UINT64_C (0xef86d60e661bcf71)},
        {&fnv_test_str[163], UINT64_C (0x9e5426c87f30ee54)},
        {&fnv_test_str[164], UINT64_C (0xf1ea8aa826fd047e)},
        {&fnv_test_str[165], UINT64_C (0x0babaf9a642cb769)},
        {&fnv_test_str[166], UINT64_C (0x4b3341d4068d012e)},
        {&fnv_test_str[167], UINT64_C (0xd15605cbc30a335c)},
        {&fnv_test_str[168], UINT64_C (0x5b21060aed8412e5)},
        {&fnv_test_str[169], UINT64_C (0x45e2cda1ce6f4227)},
        {&fnv_test_str[170], UINT64_C (0x50ae3745033ad7d4)},
        {&fnv_test_str[171], UINT64_C (0xaa4588ced46bf414)},
        {&fnv_test_str[172], UINT64_C (0xc1b0056c4a95467e)},
        {&fnv_test_str[173], UINT64_C (0x56576a71de8b4089)},
        {&fnv_test_str[174], UINT64_C (0xbf20965fa6dc927e)},
        {&fnv_test_str[175], UINT64_C (0x569f8383c2040882)},
        {&fnv_test_str[176], UINT64_C (0xe1e772fba08feca0)},
        {&fnv_test_str[177], UINT64_C (0x4ced94af97138ac4)},
        {&fnv_test_str[178], UINT64_C (0xc4112ffb337a82fb)},
        {&fnv_test_str[179], UINT64_C (0xd64a4fd41de38b7d)},
        {&fnv_test_str[180], UINT64_C (0x4cfc32329edebcbb)},
        {&fnv_test_str[181], UINT64_C (0x0803564445050395)},
        {&fnv_test_str[182], UINT64_C (0xaa1574ecf4642ffd)},
        {&fnv_test_str[183], UINT64_C (0x694bc4e54cc315f9)},
        {&fnv_test_str[184], UINT64_C (0xa3d7cb273b011721)},
        {&fnv_test_str[185], UINT64_C (0x577c2f8b6115bfa5)},
        {&fnv_test_str[186], UINT64_C (0xb7ec8c1a769fb4c1)},
        {&fnv_test_str[187], UINT64_C (0x5d5cfce63359ab19)},
        {&fnv_test_str[188], UINT64_C (0x33b96c3cd65b5f71)},
        {&fnv_test_str[189], UINT64_C (0xd845097780602bb9)},
        {&fnv_test_str[190], UINT64_C (0x84d47645d02da3d5)},
        {&fnv_test_str[191], UINT64_C (0x83544f33b58773a5)},
        {&fnv_test_str[192], UINT64_C (0x9175cbb2160836c5)},
        {&fnv_test_str[193], UINT64_C (0xc71b3bc175e72bc5)},
        {&fnv_test_str[194], UINT64_C (0x636806ac222ec985)},
        {&fnv_test_str[195], UINT64_C (0xb6ef0e6950f52ed5)},
        {&fnv_test_str[196], UINT64_C (0xead3d8a0f3dfdaa5)},
        {&fnv_test_str[197], UINT64_C (0x922908fe9a861ba5)},
        {&fnv_test_str[198], UINT64_C (0x6d4821de275fd5c5)},
        {&fnv_test_str[199], UINT64_C (0x1fe3fce62bd816b5)},
        {&fnv_test_str[200], UINT64_C (0xc23e9fccd6f70591)},
        {&fnv_test_str[201], UINT64_C (0xc1af12bdfe16b5b5)},
        {&fnv_test_str[202], UINT64_C (0x39e9f18f2f85e221)},
        {nullptr, 0U}};

    /* end of output generated by make vector.c */
    /*
     * insert the contents of vector.c above
     */
} // namespace

TEST (Fnv, StandardVectors) {
    auto test_num = 1; // test vector that failed, starting at 1.
    for (test_vector const * t = fnv_test_str; t->buf != nullptr; ++t, ++test_num) {
        std::uint64_t hval = pstore::fnv_64a_buf (t->buf, t->len);
        EXPECT_EQ (hval, fnv1a_64_vector[test_num - 1].fnv1a_64) << "failed test # " << test_num;
    }
}
// eof: unittests/pstore/test_fnv.cpp
