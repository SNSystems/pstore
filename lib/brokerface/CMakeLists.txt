#===- lib/brokerface/CMakeLists.txt ---------------------------------------===//
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

set (pstore_broker_include_dir "${PSTORE_ROOT_DIR}/include/pstore/brokerface")

set (pstore_brokerface_includes
    fifo_path.hpp
    message_type.hpp
    pubsub.hpp
    send_message.hpp
    writer.hpp
)

set (pstore_brokerface_src
    fifo_path_common.cpp
    fifo_path_posix.cpp
    fifo_path_win32.cpp
    message_type.cpp
    send_message.cpp
    writer_common.cpp
    writer_posix.cpp
    writer_win32.cpp
)

include (add_pstore)
add_pstore_library (TARGET pstore-brokerface
    NAME brokerface
    SOURCES "${pstore_brokerface_src}"
    HEADER_DIR "${pstore_broker_include_dir}"
    INCLUDES "${pstore_brokerface_includes}"
)
target_link_libraries (pstore-brokerface PUBLIC pstore-adt pstore-os pstore-support)

if (PSTORE_ENABLE_BROKER AND NOT PSTORE_EXCEPTIONS)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)

    add_pstore_library (TARGET pstore-brokerface-ex
        NAME brokerface-ex
        SOURCES "${pstore_brokerface_src}"
        HEADER_DIR "${pstore_broker_include_dir}"
        INCLUDES "${pstore_brokerface_includes}"
    )

    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
    set (PSTORE_EXCEPTIONS No)

    target_link_libraries (pstore-brokerface-ex PUBLIC pstore-adt-ex pstore-os-ex pstore-support-ex)
endif (PSTORE_ENABLE_BROKER AND NOT PSTORE_EXCEPTIONS)
