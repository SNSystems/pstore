#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/support/CMakeLists.txt ------------------------------------------===//
# Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
# All rights reserved.
#
# Developed by:
#   Toolchain Team
#   SN Systems, Ltd.
#   www.snsystems.com
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal with the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimers.
#
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimers in the
#   documentation and/or other materials provided with the distribution.
#
# - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
#   Inc. nor the names of its contributors may be used to endorse or
#   promote products derived from this Software without specific prior
#   written permission.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
#===----------------------------------------------------------------------===//


#
# -- The code setting up config.hpp
#

set (CMAKE_REQUIRED_LIBRARIES )

# I need these tests to be performed with the compiler in C++14 mode. Unfortunately, there's no
# simple way to tell cmake that so I manually enable the language by setting CMAKE_CXX_FLAGS and
# restoring it at the end. See also <https://gitlab.kitware.com/cmake/cmake/issues/15361>.
set (OLD_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
if (NOT MSVC)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif (NOT MSVC)

include (CheckCXXSourceCompiles)
include (CheckIncludeFiles)
include (CheckIncludeFileCXX)

include (TestBigEndian)
test_big_endian (PSTORE_IS_BIG_ENDIAN)

if (APPLE)
    check_include_file_cxx ("os/log.h" PSTORE_HAVE_OSLOG_H)
endif ()

check_include_files ("byteswap.h" PSTORE_HAVE_BYTESWAP_H)
check_include_files ("linux/fs.h" PSTORE_HAVE_LINUX_FS_H)
check_include_files ("linux/limits.h" PSTORE_HAVE_LINUX_LIMITS_H)
check_include_files ("sys/endian.h" PSTORE_HAVE_SYS_ENDIAN_H)
check_include_files ("sys/syscall.h" PSTORE_HAVE_SYS_SYSCALL_H)
check_include_files ("sys/time.h;sys/types.h;sys/posix_shm.h" PSTORE_HAVE_SYS_POSIX_SHM_H)
check_include_files ("syslog.h" PSTORE_HAVE_SYS_LOG_H)

check_cxx_source_compiles (
    "#include <cstdlib>
    __attribute__((noreturn)) int f () {
        std::exit(0);
    }
    int main () {}"
    PSTORE_HAVE_ATTRIBUTE_NORETURN
)

if (NOT PSTORE_DISABLE_UINT128_T)
    check_cxx_source_compiles (
        "int main () {
            __uint128_t a;
        }"
        PSTORE_HAVE_UINT128_T)
endif ()

if (PSTORE_HAVE_UINT128)
    check_cxx_source_compiles (
        #include <type_traits>
        "int main () {
            return std::is_unsigned<__uint128_t>::value;
        }"
        PSTORE_HAVE_UINT128_TRAITS_SUPPORT)
endif ()

check_cxx_source_compiles (
    "#include <string>
    int main () {
        std::string s;
        char * c = s.data ();
    }"
    PSTORE_HAVE_NON_CONST_STD_STRING_DATA)

check_cxx_source_compiles (
    "#include <map>
    int main () {
        std::map<int,int> m;
        m.insert_or_assign (1,1);
    }"
    PSTORE_STD_MAP_HAS_INSERT_OR_ASSIGN)

check_cxx_source_compiles (
    "#include <type_traits>
    int main () {
        return std::is_trivially_copyable<int>::value;
    }"
    PSTORE_HAVE_IS_TRIVIALLY_COPYABLE)

check_cxx_source_compiles (
    "#define __STDC_WANT_LIB_EXT1__  1
    #include <time.h>
    int main () {
        time_t t = time (NULL);
        struct tm buf;
        localtime_s (&buf, &t);
    }"
    PSTORE_HAVE_LOCALTIME_S)

check_cxx_source_compiles (
    "#include <time.h>
    int main () {
        time_t t = time (NULL);
        struct tm buf;
        localtime_r (&t, &buf);
    }"
    PSTORE_HAVE_LOCALTIME_R)

check_cxx_source_compiles (
    "#define __STDC_WANT_LIB_EXT1__  1
    #include <time.h>
    int main () {
        time_t t = time (NULL);
        struct tm buf;
        gmtime_s (&buf, &t);
    }"
    PSTORE_HAVE_GMTIME_S)

check_cxx_source_compiles (
    "#include <time.h>
    int main () {
        time_t t = time (NULL);
        struct tm buf;
        gmtime_r (&t, &buf);
    }"
    PSTORE_HAVE_GMTIME_R)

check_cxx_source_compiles (
    "#include <unistd.h>
    int main () { getpagesize (); }"
    PSTORE_HAVE_GETPAGESIZE)

check_cxx_source_compiles (
    "int main () { int a; int * _Nonnull b = &a; return *b; }"
    PSTORE_HAVE_NONNULL_KEYWORD
)
check_cxx_source_compiles (
    "int main () { int a; int * _Nullable b = &a; return *b; }"
    PSTORE_HAVE_NULLABLE_KEYWORD
)

check_cxx_source_compiles (
    "#include <cstdio>
    int main () { renamex_np (\"old\", \"new\", RENAME_EXCL); }"
    PSTORE_HAVE_RENAMEX_NP
)
check_cxx_source_compiles (
    "#include <linux/fs.h>
    #include <fcntl.h>
    #include <cstdio>
    int main () { renameat2 (0, \"old\", 0, \"new\", RENAME_EXCL); }"
    PSTORE_HAVE_RENAMEAT2
)
check_cxx_source_compiles (
    "#include <sys/syscall.h>
    int main () { return SYS_renameat2; }"
    PSTORE_HAVE_SYS_renameat2
)


# The time members of struct stat might be called st_Xtimespec (of type struct timespec)
# or st_Xtime (and be of type time_t).
check_cxx_source_compiles (
    "#include <sys/stat.h>
    int main () {
        struct stat buf;
        struct timespec a = buf.st_atimespec;
    }"
    PSTORE_STAT_TIMESPEC)


set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package (Threads)
set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})

check_cxx_source_compiles (
    "#include <pthread.h>
    #ifdef __FreeBSD__
    #include <pthread_np.h>
    #endif
    int main () {
        pthread_setname_np (pthread_self (), \"foo\");
    }"
    PSTORE_PTHREAD_SETNAME_NP_2_ARGS)

check_cxx_source_compiles (
     "#include <pthread.h>
     #ifdef __FreeBSD__
     #include <pthread_np.h>
     #endif
     int main () {
         pthread_setname_np (\"foo\");
     }"
     PSTORE_PTHREAD_SETNAME_NP_1_ARG)

check_cxx_source_compiles (
    "#include <pthread.h>
    #ifdef __FreeBSD__
    #include <pthread_np.h>
    #endif
    int main () {
        pthread_set_name_np (pthread_self (), \"foo\");
    }"
    PSTORE_PTHREAD_SET_NAME_NP)
set (CMAKE_REQUIRED_LIBRARIES )

check_cxx_source_compiles (
    "#include <mach-o/dyld.h>
    int main () {
        return _NSGetExecutablePath (0, 0);
    }"
    PSTORE_HAVE_NSGETEXECUTABLEPATH)


configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"
    "${PSTORE_CONFIG_DIR}/pstore/config/config.hpp"
    ESCAPE_QUOTES
)

set (CMAKE_CXX_FLAGS "${OLD_CMAKE_CXX_FLAGS}")

#
# -- Thats's the end of the code setting up config.hpp
#

include (add_pstore)
include (clang_tidy)

set (PSTORE_SUPPORT_INCLUDES
    aligned.hpp
    array_elements.hpp
    base64.hpp
    bit_count.hpp
    bit_field.hpp
    ctype.hpp
    error.hpp
    fnv.hpp
    gsl.hpp
    head_revision.hpp
    inherit_const.hpp
    ios_state.hpp
    max.hpp
    maybe.hpp
    parallel_for_each.hpp
    pointee_adaptor.hpp
    portab.hpp
    quoted.hpp
    random.hpp
    round2.hpp
    scope_guard.hpp
    uint128.hpp
    unsigned_cast.hpp
    utf.hpp
    varint.hpp
)
set (PSTORE_SUPPORT_LIB_SRC
    config.hpp.in
    "${PSTORE_CONFIG_DIR}/pstore/config/config.hpp"

    error.cpp
    fnv.cpp
    uint128.cpp
    utf.cpp
    utf_win32.cpp
)

add_pstore_library (
    NAME support
    SOURCES "${PSTORE_SUPPORT_LIB_SRC}"
    HEADER_DIR "${PSTORE_ROOT_DIR}/include/pstore/support"
    INCLUDES "${PSTORE_SUPPORT_INCLUDES}"
)
target_link_libraries (pstore-support PUBLIC ${CMAKE_THREAD_LIBS_INIT})
add_clang_tidy_target (pstore-support)

if (NOT PSTORE_IS_INSIDE_LLVM)
    install (
        FILES "${PSTORE_CONFIG_DIR}/pstore/config/config.hpp"
        DESTINATION include/pstore/config
    )
endif ()

# Add the exception-handling-enabled pstore-support-ex if we're building inside LLVM
if (NOT PSTORE_EXCEPTIONS)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)
    add_pstore_library (
        TARGET pstore-support-ex
        NAME support
        SOURCES "${PSTORE_SUPPORT_LIB_SRC}"
        HEADER_DIR "${PSTORE_ROOT_DIR}/include/pstore/support"
        INCLUDES "${PSTORE_SUPPORT_INCLUDES}"
    )
    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
    set (PSTORE_EXCEPTIONS No)

    target_link_libraries (pstore-support-ex PUBLIC ${CMAKE_THREAD_LIBS_INIT})
endif (NOT PSTORE_EXCEPTIONS)

if (PSTORE_BITCODE)
    add_pstore_library (
        TARGET pstore-support-bc
        NAME support
        SOURCES "${PSTORE_SUPPORT_LIB_SRC}"
        HEADER_DIR "${PSTORE_ROOT_DIR}/include/pstore/support"
        INCLUDES "${PSTORE_SUPPORT_INCLUDES}"
    )
    target_compile_options (pstore-support-bc PRIVATE
        -emit-llvm
        -target x86_64-pc-linux-gnu
        -fsanitize=signed-integer-overflow
        -fsanitize=unsigned-integer-overflow
        -fno-threadsafe-statics
    )
    target_link_libraries (pstore-support-bc PUBLIC ${CMAKE_THREAD_LIBS_INIT})
endif (PSTORE_BITCODE)
