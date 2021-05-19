#===- lib/broker/CMakeLists.txt -------------------------------------------===//
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

if (NOT PSTORE_ENABLE_BROKER)
    message (STATUS "pstore broker library is excluded (PSTORE_ENABLE_BROKER)")
else ()
    set (pstore_broker_include_dir "${PSTORE_ROOT_DIR}/include/pstore/broker")

    set (pstore_broker_includes
        bimap.hpp
        command.hpp
        gc.hpp
        globals.hpp
        internal_commands.hpp
        intrusive_list.hpp
        message_pool.hpp
        message_queue.hpp
        parser.hpp
        pointer_compare.hpp
        quit.hpp
        read_loop.hpp
        recorder.hpp
        scavenger.hpp
        spawn.hpp
        uptime.hpp
    )

    set (pstore_broker_lib_src
        command.cpp
        gc_common.cpp
        gc_posix.cpp
        gc_win32.cpp
        globals.cpp
        internal_commands.cpp
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
            SOURCES "${pstore_broker_lib_src}"
            HEADER_DIR "${pstore_broker_include_dir}"
            INCLUDES "${pstore_broker_includes}"
        )
        target_link_libraries (pstore-broker PUBLIC
            pstore-adt-ex
            pstore-brokerface-ex
            pstore-http-ex
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
            SOURCES "${pstore_broker_lib_src}"
            HEADER_DIR "${pstore_broker_include_dir}"
            INCLUDES "${pstore_broker_includes}"
        )
        target_link_libraries (pstore-broker PUBLIC
            pstore-adt
            pstore-brokerface
            pstore-http
            pstore-json-lib
            pstore-os
            pstore-support
        )
    endif (NOT PSTORE_EXCEPTIONS)

endif (NOT PSTORE_ENABLE_BROKER)
