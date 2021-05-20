#===- lib/exchange/CMakeLists.txt -----------------------------------------===//
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

set (pstore_exchange_include_dir "${PSTORE_ROOT_DIR}/include/pstore/exchange/")
set (pstore_exchange_includes
    export.hpp
    export_compilation.hpp
    export_emit.hpp
    export_fixups.hpp
    export_fragment.hpp
    export_ostream.hpp
    export_paths.hpp
    export_section.hpp
    export_strings.hpp
    import_bss_section.hpp
    import_compilation.hpp
    import_context.hpp
    import_debug_line_header.hpp
    import_debug_line_section.hpp
    import_error.hpp
    import_fixups.hpp
    import_fragment.hpp
    import_generic_section.hpp
    import_linked_definitions_section.hpp
    import_non_terminals.hpp
    import_root.hpp
    import_rule.hpp
    import_section_to_importer.hpp
    import_strings_array.hpp
    import_strings.hpp
    import_terminals.hpp
    import_transaction.hpp
    import_uuid.hpp
)
set (pstore_exchange_sources
    export.cpp
    export_compilation.cpp
    export_emit.cpp
    export_fixups.cpp
    export_fragment.cpp
    export_ostream.cpp
    export_paths.cpp
    export_strings.cpp
    import_compilation.cpp
    import_debug_line_header.cpp
    import_error.cpp
    import_fixups.cpp
    import_fragment.cpp
    import_root.cpp
    import_rule.cpp
    import_strings.cpp
    import_strings_array.cpp
    import_terminals.cpp
    import_uuid.cpp
)

add_pstore_library (
    TARGET pstore-exchange
    NAME exchange
    SOURCES "${pstore_exchange_sources}"
    HEADER_DIR "${pstore_exchange_include_dir}"
    INCLUDES "${pstore_exchange_includes}"
)
target_link_libraries (pstore-exchange PUBLIC pstore-core pstore-json-lib pstore-mcrepo)
add_clang_tidy_target (pstore-exchange)
