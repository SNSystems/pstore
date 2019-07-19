#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/broker/CMakeLists.txt -------------------------------------------===//
# Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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

if (NOT PSTORE_ENABLE_BROKER)
    message (STATUS "pstore broker library is excluded (PSTORE_ENABLE_BROKER)")
else ()
    set (pstore_broker_include_dir "${PSTORE_ROOT_DIR}/include/pstore/broker")

    set (pstore_broker_includes
        "${pstore_broker_include_dir}/bimap.hpp"
        "${pstore_broker_include_dir}/command.hpp"
        "${pstore_broker_include_dir}/gc.hpp"
        "${pstore_broker_include_dir}/globals.hpp"
        "${pstore_broker_include_dir}/intrusive_list.hpp"
        "${pstore_broker_include_dir}/message_pool.hpp"
        "${pstore_broker_include_dir}/message_queue.hpp"
        "${pstore_broker_include_dir}/parser.hpp"
        "${pstore_broker_include_dir}/pointer_compare.hpp"
        "${pstore_broker_include_dir}/quit.hpp"
        "${pstore_broker_include_dir}/read_loop.hpp"
        "${pstore_broker_include_dir}/recorder.hpp"
        "${pstore_broker_include_dir}/scavenger.hpp"
        "${pstore_broker_include_dir}/spawn.hpp"
        "${pstore_broker_include_dir}/uptime.hpp"
    )

    set (pstore_broker_lib_src
        command.cpp
        gc_common.cpp
        gc_posix.cpp
        gc_win32.cpp
        globals.cpp
        message_pool.cpp
        parser.cpp
        quit.cpp
        read_loop_posix.cpp
        read_loop_win32.cpp
        recorder.cpp
        scavenger.cpp
        spawn_posix.cpp
        spawn_win32.cpp
        uptime.cpp
    )

    include (add_pstore)
    if (NOT PSTORE_EXCEPTIONS)
        set (LLVM_REQUIRES_EH Yes)
        set (LLVM_REQUIRES_RTTI Yes)
        set (PSTORE_EXCEPTIONS Yes)
        add_pstore_library (
            TARGET pstore-broker
            NAME broker
            SOURCES ${pstore_broker_lib_src}
            INCLUDES ${pstore_broker_includes}
        )
        target_link_libraries (pstore-broker PUBLIC
            pstore-broker-intf-ex
            pstore-httpd-lib-ex
            pstore-json-lib-ex
            pstore-os-ex
            pstore-support-ex
        )
        set (LLVM_REQUIRES_EH No)
        set (LLVM_REQUIRES_RTTI No)
        set (PSTORE_EXCEPTIONS No)
    else ()
        add_pstore_library (
            TARGET pstore-broker
            NAME broker
            SOURCES ${pstore_broker_lib_src}
            INCLUDES ${pstore_broker_includes}
        )
        target_link_libraries (pstore-broker PUBLIC
            pstore-broker-intf
            pstore-httpd-lib
            pstore-json-lib
            pstore-os
            pstore-support
        )
    endif (NOT PSTORE_EXCEPTIONS)

endif (NOT PSTORE_ENABLE_BROKER)
