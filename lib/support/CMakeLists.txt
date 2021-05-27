#===- lib/support/CMakeLists.txt ------------------------------------------===//
#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===----------------------------------------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
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
check_include_files ("linux/unistd.h" PSTORE_HAVE_LINUX_UNISTD_H)
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
        pthread_setname_np (pthread_self (), \"foo\", nullptr);
    }"
    PSTORE_PTHREAD_SETNAME_NP_3_ARGS)

check_cxx_source_compiles (
    "#include <pthread.h>
    #ifdef __FreeBSD__
    #include <pthread_np.h>
    #endif
    int main () {
        pthread_set_name_np (pthread_self (), \"foo\");
    }"
    PSTORE_PTHREAD_SET_NAME_NP)

check_cxx_source_compiles (
    "#include <pthread.h>
    #ifdef __FreeBSD__
    #include <pthread_np.h>
    #endif
    int main () {
        pthread_getname_np (pthread_self (), \"foo\", 4);
    }"
    PSTORE_PTHREAD_GETNAME_NP)

check_cxx_source_compiles (
    "#include <pthread.h>
    #ifdef __FreeBSD__
    #include <pthread_np.h>
    #endif
    int main () {
        pthread_get_name_np (pthread_self (), \"foo\", 4);
    }"
    PSTORE_PTHREAD_GET_NAME_NP)

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
install (FILES
    "${PSTORE_CONFIG_DIR}/pstore/config/config.hpp"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/pstore/config"
    COMPONENT pstore
)
set (CMAKE_CXX_FLAGS "${OLD_CMAKE_CXX_FLAGS}")

#
# -- That's the end of the code setting up config.hpp
#

find_package (Backtrace)
set (PSTORE_HAVE_BACKTRACE  ${Backtrace_FOUND})

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/backtrace.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/backtrace.hpp"
    ESCAPE_QUOTES
)

include (add_pstore)
include (clang_tidy)

set (PSTORE_SUPPORT_INCLUDES
    aligned.hpp
    array_elements.hpp
    assert.hpp
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

    "${CMAKE_CURRENT_SOURCE_DIR}/backtrace.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/backtrace.hpp"

    assert.cpp
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

function (pstore_config_support_library target)
    target_link_libraries (${target} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
    target_include_directories (${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    if (Backtrace_FOUND)
        target_include_directories (${target} PRIVATE ${Backtrace_INCLUDE_DIRS})
        target_link_libraries (${target} PRIVATE ${Backtrace_LIBRARY})
    endif ()
endfunction (pstore_config_support_library)

pstore_config_support_library (pstore-support)
add_clang_tidy_target (pstore-support)

if (PSTORE_ENABLE_BROKER AND NOT PSTORE_EXCEPTIONS)
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

    pstore_config_support_library (pstore-support-ex)
endif (PSTORE_ENABLE_BROKER AND NOT PSTORE_EXCEPTIONS)

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
    set_target_properties (pstore-support-bc PROPERTIES
        CXX_STANDARD "${pstore_cxx_version}"
        CXX_STANDARD_REQUIRED Yes
    )
    pstore_config_support_library (pstore-support-bc)
endif (PSTORE_BITCODE)
