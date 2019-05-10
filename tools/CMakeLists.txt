#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===- tools/CMakeLists.txt ------------------------------------------------===//
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

add_subdirectory (brokerd)
add_subdirectory (broker_poker) # A utility for exercising the broker agent.
add_subdirectory (diff)         # Dumps diff between two pstore revisions as YAML.
add_subdirectory (dump)         # Dumps pstore contents as YAML.
add_subdirectory (genromfs)     # Converts a local directory tree to romfs.
add_subdirectory (hamt_test)    # A utility to check the HAMT index
add_subdirectory (httpd)        # A host for the broker's HTTP server
add_subdirectory (index_structure) # Dumps pstore index structures as GraphViz DOT graphs.
add_subdirectory (inserter)     # A utility to exercise the digest index
add_subdirectory (json)         # A small wrapper for the JSON parser library.
add_subdirectory (mangle)       # A simple file fuzzing utility
add_subdirectory (read)         # A utility for reading the write or strings index. 
add_subdirectory (sieve)        # A utility to generate data for the system tests
add_subdirectory (vacuum)       # Data store garbage collector utility
add_subdirectory (write)
