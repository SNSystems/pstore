#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- tools/brokerd/service/init/CMakeLists.txt ---------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//
set (PSTORE_BROKERD "${CMAKE_INSTALL_PREFIX}/bin/pstore-brokerd")

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/pstore-brokerd.init"
    "${CMAKE_CURRENT_BINARY_DIR}/pstore-brokerd.init"
    ESCAPE_QUOTES
    @ONLY
)
install (
    FILES "${CMAKE_CURRENT_BINARY_DIR}/pstore-brokerd.init"
    RENAME pstore-brokerd
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    DESTINATION etc/init.d
    COMPONENT pstore
)
