# *                                             *
# *   ___ ___  _ __ ___  _ __ ___   ___  _ __   *
# *  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \| '_ \  *
# * | (_| (_) | | | | | | | | | | | (_) | | | | *
# *  \___\___/|_| |_| |_|_| |_| |_|\___/|_| |_| *
# *                                             *
# ===- system_tests/broker/common.py --------------------------------------===//
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
from __future__ import print_function
import os
import platform
import sys

PLATFORM_NAME = platform.system()
IS_WINDOWS = PLATFORM_NAME == 'Windows'
IS_CYGWIN = PLATFORM_NAME.startswith('CYGWIN_NT-')


def executable(path):
    """
    On a Windows or Cygwin host, adds the Windows .exe file extension for an 
    executable if it isn't there already.

    :param path: The path or name of an executable.
    :return: The path or name of an executable with the .exe extension.
    """

    if IS_WINDOWS or IS_CYGWIN:
        root, extension = os.path.splitext(path)
        if extension != '.exe' and extension != '.':
            extension += '.exe'
            path = root + extension
    return path


def pipe_root_dir():
    return r'\\.\pipe' if IS_WINDOWS or IS_CYGWIN else '/tmp'


def report_error(argv0, tool_name, timed_process):
    ok = True
    if timed_process.did_timeout():
        print("%s: %s timeout" % (argv0, tool_name,), file=sys.stderr)
        ok = False
    ex = timed_process.exception()
    if ex is not None:
        print("%s: %s exception: %s" % (argv0, tool_name, ex), file=sys.stderr)
        ok = False
    return ok
