#===- tools/CMakeLists.txt ------------------------------------------------===//
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

add_subdirectory (brokerd)
add_subdirectory (broker_poker) # A utility for exercising the broker agent
add_subdirectory (diff)         # Dumps diff between two pstore revisions as YAML
add_subdirectory (dump)         # Dumps pstore contents as YAML
add_subdirectory (export)       # Exports a pstore file as JSON
add_subdirectory (genromfs)     # Converts a local directory tree to romfs
add_subdirectory (hamt_test)    # A utility to check the HAMT index
add_subdirectory (httpd)        # A host for the broker's HTTP server
add_subdirectory (import)
add_subdirectory (index_stats)  # Dumps pstore index statistics
add_subdirectory (index_structure) # Dumps pstore index structures as GraphViz DOT graphs
add_subdirectory (inserter)     # A utility to exercise the digest index
add_subdirectory (json)         # A small wrapper for the JSON parser library
add_subdirectory (lock_test)    # Test the global transaction lock
add_subdirectory (mangle)       # A simple file fuzzing utility
add_subdirectory (read)         # A utility for reading the write or strings index
add_subdirectory (sieve)        # A utility to generate data for the system tests
add_subdirectory (vacuum)       # Data store garbage collector utility
add_subdirectory (write)
