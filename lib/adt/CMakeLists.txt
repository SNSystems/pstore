#===- lib/adt/CMakeLists.txt ----------------------------------------------===//
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
set (pstore_adt_lib_src
    sstring_view.cpp
)
set (pstore_adt_include_dir "${PSTORE_ROOT_DIR}/include/pstore/adt")
set (pstore_adt_lib_includes
    chunked_sequence.hpp
    error_or.hpp
    small_vector.hpp
    sparse_array.hpp
    sstring_view.hpp
    utility.hpp
)
      
add_pstore_library (
    NAME adt
    SOURCES ${pstore_adt_lib_src}
    HEADER_DIR "${pstore_adt_include_dir}"
    INCLUDES ${pstore_adt_lib_includes}
)
target_link_libraries (pstore-adt PUBLIC pstore-support)
add_clang_tidy_target (pstore-adt)

if (PSTORE_ENABLE_BROKER AND NOT PSTORE_EXCEPTIONS)
    set (LLVM_REQUIRES_EH Yes)
    set (LLVM_REQUIRES_RTTI Yes)
    set (PSTORE_EXCEPTIONS Yes)
    add_pstore_library (
        TARGET pstore-adt-ex
        NAME adt
        SOURCES ${pstore_adt_lib_src}
        HEADER_DIR "${pstore_adt_include_dir}"
        INCLUDES ${pstore_adt_lib_includes}
    )
    set (LLVM_REQUIRES_EH No)
    set (LLVM_REQUIRES_RTTI No)
    set (PSTORE_EXCEPTIONS No)

    target_link_libraries (pstore-adt-ex PUBLIC pstore-support-ex)
endif (PSTORE_ENABLE_BROKER AND NOT PSTORE_EXCEPTIONS)

if (PSTORE_BITCODE)
    add_pstore_library (
        TARGET pstore-adt-bc
        NAME os
        SOURCES ${pstore_adt_lib_src}
        HEADER_DIR "${pstore_adt_include_dir}"
        INCLUDES ${pstore_adt_lib_includes}
    )
    target_compile_options (pstore-adt-bc PRIVATE
        -emit-llvm
        -target x86_64-pc-linux-gnu
        -fsanitize=signed-integer-overflow
        -fsanitize=unsigned-integer-overflow
        -fno-threadsafe-statics
    )
    set_target_properties (pstore-adt-bc PROPERTIES
        CXX_STANDARD "${pstore_cxx_version}"
        CXX_STANDARD_REQUIRED Yes
    )
    target_link_libraries (pstore-adt-bc PUBLIC pstore-support-bc)
endif (PSTORE_BITCODE)
