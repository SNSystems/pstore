#===- lib/json/CMakeLists.txt ---------------------------------------------===//
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

include (add_pstore)

set (pstore_json_include_dir "${PSTORE_ROOT_DIR}/include/pstore/json/")
set (pstore_json_includes
    dom_types.hpp
    json.hpp
    json_error.hpp
    utility.hpp
)
set (pstore_json_sources
    json_error.cpp
    utility.cpp
)

add_pstore_library (
    TARGET pstore-json-lib
    NAME json
    SOURCES "${pstore_json_sources}"
    HEADER_DIR "${pstore_json_include_dir}"
    INCLUDES "${pstore_json_includes}"
)
target_link_libraries (pstore-json-lib PUBLIC pstore-adt pstore-support)
add_clang_tidy_target (pstore-json-lib)

if (PSTORE_ENABLE_BROKER AND NOT PSTORE_EXCEPTIONS)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)

    add_pstore_library (
        TARGET pstore-json-lib-ex
        NAME json
        SOURCES "${pstore_json_sources}"
        HEADER_DIR "${pstore_json_include_dir}"
        INCLUDES "${pstore_json_includes}"
    )

    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
    set (PSTORE_EXCEPTIONS No)

    target_link_libraries (pstore-json-lib-ex PUBLIC pstore-adt-ex pstore-support-ex)
endif (PSTORE_ENABLE_BROKER AND NOT PSTORE_EXCEPTIONS)


if (PSTORE_BITCODE)
    add_pstore_library (
        TARGET pstore-json-lib-bc
        NAME json
        SOURCES "${pstore_json_sources}"
        HEADER_DIR "${pstore_json_include_dir}"
        INCLUDES "${pstore_json_includes}"
    )
    target_compile_options (pstore-json-lib-bc PRIVATE
        -emit-llvm
        -target x86_64-pc-linux-gnu
        -fsanitize=signed-integer-overflow
        -fsanitize=unsigned-integer-overflow
        -fno-threadsafe-statics
    )
    set_target_properties (pstore-json-lib-bc PROPERTIES
        CXX_STANDARD "${pstore_cxx_version}"
        CXX_STANDARD_REQUIRED Yes
    )
    target_link_libraries (pstore-json-lib-bc PUBLIC pstore-adt-bc pstore-support-bc)
endif (PSTORE_BITCODE)
