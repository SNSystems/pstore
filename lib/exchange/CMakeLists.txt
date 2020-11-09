#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- lib/exchange/CMakeLists.txt -----------------------------------------===//
# Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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

include (add_pstore)

set (pstore_exchange_include_dir "${PSTORE_ROOT_DIR}/include/pstore/exchange/")
set (pstore_exchange_includes
    export.hpp
    export_compilation.hpp
    export_emit.hpp
    export_fixups.hpp
    export_fragment.hpp
    export_ostream.hpp
    export_names.hpp
    export_section.hpp
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
    import_names.hpp
    import_names_array.hpp
    import_non_terminals.hpp
    import_root.hpp
    import_rule.hpp
    import_section_to_importer.hpp
    import_terminals.hpp
    import_transaction.hpp
    import_uuid.hpp
)
set (pstore_exchange_sources
    export.cpp
    export_emit.cpp
    export_fixups.cpp
    export_fragment.cpp
    export_names.cpp
    export_ostream.cpp
    import_compilation.cpp
    import_error.cpp
    import_fixups.cpp
    import_fragment.cpp
    import_names.cpp
    import_root.cpp
    import_rule.cpp
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
