#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- tools/brokerd/service/win32/CMakeLists.txt --------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#===----------------------------------------------------------------------===//

include (add_pstore)

if (PSTORE_IS_INSIDE_LLVM)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
endif ()

add_pstore_tool (pstore-broker-service
    main.cpp
    broker_service.cpp
    broker_service.hpp
    service_base.cpp
    service_base.hpp
    service_installer.cpp
    service_installer.hpp
)

if (PSTORE_IS_INSIDE_LLVM)
    target_link_libraries (pstore-broker-service PRIVATE pstore-command-line-ex)
    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
else ()
    target_link_libraries (pstore-broker-service PRIVATE pstore-command-line)
endif ()
