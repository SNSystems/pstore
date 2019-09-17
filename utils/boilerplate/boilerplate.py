#!/usr/bin/env python
# *  _           _ _                 _       _        *
# * | |__   ___ (_) | ___ _ __ _ __ | | __ _| |_ ___  *
# * | '_ \ / _ \| | |/ _ \ '__| '_ \| |/ _` | __/ _ \ *
# * | |_) | (_) | | |  __/ |  | |_) | | (_| | ||  __/ *
# * |_.__/ \___/|_|_|\___|_|  | .__/|_|\__,_|\__\___| *
# *                           |_|                     *
# ===- utils/boilerplate/boilerplate.py -----------------------------------===//
#  Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
#  All rights reserved.
#
#  Developed by:
#    Toolchain Team
#    SN Systems, Ltd.
#    www.snsystems.com
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the
#  "Software"), to deal with the Software without restriction, including
#  without limitation the rights to use, copy, modify, merge, publish,
#  distribute, sublicense, and/or sell copies of the Software, and to
#  permit persons to whom the Software is furnished to do so, subject to
#  the following conditions:
#
#  - Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimers.
#
#  - Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimers in the
#    documentation and/or other materials provided with the distribution.
#
#  - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
#    Inc. nor the names of its contributors may be used to endorse or
#    promote products derived from this Software without specific prior
#    written permission.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#  IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
#  ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#  SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
# ===----------------------------------------------------------------------===//

"""
 A small utility tool to stamp boilerplate text onto source files. It uses
 an external tool "FIGlet" (www.figlet.org) to generate banner text (although
 this can be disabled).
"""

from __future__ import print_function

import argparse
import os.path
import subprocess
import sys

license_text = \
'''Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
All rights reserved.

Developed by:
  Toolchain Team
  SN Systems, Ltd.
  www.snsystems.com

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal with the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimers.

- Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimers in the
  documentation and/or other materials provided with the distribution.

- Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
  Inc. nor the names of its contributors may be used to endorse or
  promote products derived from this Software without specific prior
  written permission.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
'''


def strip_lines(lines, index, comment_str):
    """Removes leading and trailing comment lines from the file."""

    # Remove blank lines
    while len(lines) > 0 and (lines[index] == '' or lines[index] == '\n'):
        del lines[index]

    # Remove comment lines.
    stop_str = comment_str + comment_str[0]
    while len(lines) > 0 and lines[index].startswith(comment_str) and not lines[index].startswith(stop_str):
        del lines[index]
    return lines


def strip_leading_and_trailing_lines(lines, comment):
    """
    Removes and leading and trailing blank lines and comments.

    :param lines: An array of strings containing the lines to be stripped.
    :param comment: The block comment character string.
    :return: An updated array of lines.
    """

    comment = comment.strip()
    return strip_lines(strip_lines(lines, 0, comment), -1, comment)


def remove_string_prefix(s, prefix):
    if s.startswith(prefix):
        s = s[len(prefix):]
    return s


def remove_string_suffix(s, suffix):
    if s.endswith(suffix):
        s = s[:-len(suffix)]
    return s


def split_extension(path):
    """A wrapped around splitext() which will delete a '.in' extension. This convention is used for files
    being fed as the template file to cmake's configure_file() command; the resulting file has the same
    name but without the .in part."""

    result = os.path.splitext(path)
    if result[1] == '.in':
        result = os.path.splitext(result[0])
    return result


def tu_name_from_path(path):
    name = split_extension(os.path.basename(path))[0]
    name = remove_string_prefix(name, 'test_')
    name = remove_string_suffix(name, '_win32')
    name = remove_string_suffix(name, '_posix')
    return name.replace('_', ' ')


def figlet(name, comment_char):
    command = 'figlet "' + name + '"'
    if sys.version_info[0] >= 3:
        out = subprocess.getoutput(command)
    else:
        out = subprocess.check_output(command, shell=True)
    comments = [comment_char + '* ' + l + ' *' + '\n' for l in out.splitlines(False)]
    return comments


def get_path_line(path, comment_char):
    path_line_suffix = '===//'
    path_line = comment_char + '===- ' + path + ' '
    path_line += '-' * (80 - len(path_line) - len(path_line_suffix)) + path_line_suffix
    return [path_line + '\n']


def get_license(comment_char):
    license = [comment_char + ' ' + l for l in license_text.splitlines(False)]
    license = [l.rstrip(' ') + '\n' for l in license]
    return license


COMMENT_MAPPING = {
    '.cpp': '//',
    '.c': '//',
    '.C': '//',
    '.cxx': '//',
    '.txt': '#',  # for CMake!
    '.h': '//',
    '.hpp': '//',
    '.py': '# ',
}


def boilerplate(path, base_path, comment_char=None, figlet_enabled=True):
    path = os.path.abspath(path)
    base_path = os.path.abspath(base_path)

    if comment_char is None:
        ext = split_extension(path)[1]
        comment_char = COMMENT_MAPPING.get(ext, '#')

    if not os.path.isdir(base_path):
        raise RuntimeError('base path must be a directory')

    if base_path[-1] != os.sep and base_path[-1] != os.altsep:
        base_path += os.sep

    if path[:len(base_path)] != base_path:
        raise RuntimeError('path (%s) was not inside base-path (%s)' % (path, base_path))

    subpath = path[len(base_path):]
    if subpath.endswith('.in'):
        subpath = subpath[:-len('.in')]

    with open(path) as f:
        lines = f.readlines()

    shebang = lines[0] if len(lines) > 0 and lines[0][:2] == '#!' else None
    if shebang is not None:
        del lines[0]

    # Remove any leading and trailing blank lines and comments
    lines = strip_leading_and_trailing_lines(lines, comment_char)

    tu_name = tu_name_from_path(subpath)
    prolog = []
    if shebang is not None:
        prolog += [shebang]

    if figlet_enabled:
        prolog += figlet(tu_name, comment_char)

    prolog += get_path_line(subpath, comment_char)
    prolog += get_license(comment_char)
    prolog += [comment_char + '===----------------------------------------------------------------------===//\n']
    return prolog + lines


def boilerplate_out(path, base_path, comment_char=None, inplace=False, figlet_enabled=True):
    lines = boilerplate(path, base_path, comment_char, figlet_enabled)
    outfile = open(path, 'w') if inplace else sys.stdout
    for l in lines:
        print(l, end='', file=outfile)


def main(args=sys.argv[1:]):
    parser = argparse.ArgumentParser(description='Generate source file license boilerplate')
    parser.add_argument('source_file', help='The source file to be processed')
    parser.add_argument('--base-path', help='The base path to which path names are relative', default=os.getcwd())
    parser.add_argument('--comment-char', help='The character(s) used to begin a comment.')
    parser.add_argument('--no-figlet',
                        help='Disables generation of the TU banner name (using the external "figlet" utility',
                        dest='figlet_enabled', action='store_false')
    parser.add_argument('-i', dest='inplace', action='store_true', help='Inplace edit file.')
    options = parser.parse_args(args)

    boilerplate_out(options.source_file, options.base_path, options.comment_char, options.inplace,
                    options.figlet_enabled)
    return 0


if __name__ == '__main__':
    sys.exit(main())
