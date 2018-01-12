#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/pstore_support/CMakeLists.txt -----------------------------------===//
# Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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

include (CheckCXXSourceCompiles)
include (CheckIncludeFiles)

if (APPLE)
    check_include_files ("asl.h" PSTORE_HAVE_ASL_H)
endif ()
check_include_files ("syslog.h" PSTORE_HAVE_SYS_LOG_H)

set (CMAKE_REQUIRED_LIBRARIES )
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


set (PSTORE_INCLUDE_DIR "${PSTORE_ROOT_DIR}/include")
set (PSTORE_SUPPORT_INCLUDE_DIR "${PSTORE_INCLUDE_DIR}/pstore_support")

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/support_config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/support_config.hpp"
    ESCAPE_QUOTES
)

set (PSTORE_SUPPORT_LIB_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/support_config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/support_config.hpp"

    "${PSTORE_SUPPORT_INCLUDE_DIR}/ctype.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/error.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/file.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/file_posix.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/file_win32.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/gsl.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/logging.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/maybe.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/path.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/portab.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/process_file_name.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/quoted_string.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/random.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/rotating_log.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/signal_cv.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/signal_helpers.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/small_vector.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/thread.hpp"
    "${PSTORE_SUPPORT_INCLUDE_DIR}/utf.hpp"
    error.cpp
    file.cpp
    file_posix.cpp
    file_win32.cpp
    logging.cpp
    path.cpp
    process_file_name_posix.cpp
    process_file_name_win32.cpp
    signal_cv_posix.cpp
    signal_cv_win32.cpp
    signal_helpers.cpp
    thread.cpp
    utf.cpp
    utf_win32.cpp
)

include (add_pstore)
if (PSTORE_IS_INSIDE_LLVM)
    add_pstore_library (pstore-support-lib STATIC ${PSTORE_SUPPORT_LIB_SRC})

    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)

    add_pstore_library (pstore-support-lib-ex STATIC ${PSTORE_SUPPORT_LIB_SRC})
    include_directories (BEFORE "${PSTORE_INCLUDE_DIR}")

    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
else ()
    add_pstore_library (pstore-support-lib ${PSTORE_SUPPORT_LIB_SRC})
    target_include_directories (pstore-support-lib PUBLIC "${PSTORE_INCLUDE_DIR}")
endif (PSTORE_IS_INSIDE_LLVM)

target_link_libraries (pstore-support-lib PUBLIC ${CMAKE_THREAD_LIBS_INIT})

# eof: lib/pstore_support/CMakeLists.txt
