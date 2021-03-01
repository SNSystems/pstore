#!/usr/bin/env python
# ===- utils/boilerplate/all.py -------------------------------------------===//
# *        _ _  *
# *   __ _| | | *
# *  / _` | | | *
# * | (_| | | | *
# *  \__,_|_|_| *
# *             *
# ===----------------------------------------------------------------------===//
#
#  Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
#  See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
#  information.
#  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# ===----------------------------------------------------------------------===//
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


def main():
    base_path = os.getcwd()
    patterns = [
        '*.cmake',
        '*.cpp',
        '*.h',
        '*.hpp',
        '*.hpp.in',
        '*.js',
        '*.py',
        'CMakeLists.txt',
        'Doxyfile.in'
    ]

    # The collection of directories into which this utility will not descend.
    exclude_dirs = frozenset(itertools.chain([
        '.git',
        '3rd_party',
        'lit',
        'node_modules'
    ], glob.iglob('build_*')))
    exclude_files = frozenset([
        'fnv.hpp',
        'fnv.cpp',
        'test_fnv.cpp'
    ])

    all_paths = []
    for root, dirs, files in os.walk(base_path, topdown=True):
        dirs[:] = [d for d in dirs if d not in exclude_dirs]
        for pattern in patterns:
            all_paths += [os.path.join(root, file_name) for file_name in files if
                          fnmatch.fnmatch(file_name, pattern) and file_name not in exclude_files]

    for path in all_paths:
        boilerplate.boilerplate_out(path=path, base_path=base_path, inplace=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
