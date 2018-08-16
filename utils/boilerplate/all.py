#!/usr/bin/env python
#*        _ _  *
#*   __ _| | | *
#*  / _` | | | *
#* | (_| | | | *
#*  \__,_|_|_| *
#*             *
#===- utils/boilerplate/all.py --------------------------------------------===//
# Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
"""
A simple utility which applies standard "boilerplate" to an entire directory tree of files. Only files whose
names match on of a set of globbing patterns will be modified.
"""

from __future__ import print_function

import fnmatch
import glob
import itertools
import os
import sys

import boilerplate


def main ():
    base_path = os.getcwd ()
    patterns = ["*.h", "*.hpp", "*.hpp.in", "*.cpp", "CMakeLists.txt", "*.cmake", "*.py", "Doxyfile.in"]

    # The collection of directories into which this utility will not descend.
    exclude_dirs = frozenset (itertools.chain ([".git", "3rd_party", "lit"], glob.iglob ("build_*")))
    exclude_files = frozenset (["fnv.hpp", "fnv.cpp", "test_fnv.cpp"])

    all_paths = []
    for root, dirs, files in os.walk (base_path, topdown=True):
        dirs [:] = [d for d in dirs if d not in exclude_dirs]
        for pattern in patterns:
            all_paths += [os.path.join (root, file_name) for file_name in files if
                          fnmatch.fnmatch (file_name, pattern) and not file_name in exclude_files]

    for path in all_paths:
        boilerplate.boilerplate_out (path=path, base_path=base_path, inplace=True)
    return 0


if __name__ == '__main__':
    sys.exit (main ())

