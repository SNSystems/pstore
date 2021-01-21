# *                                             *
# *   ___ ___  _ __ ___  _ __ ___   ___  _ __   *
# *  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \| '_ \  *
# * | (_| (_) | | | | | | | | | | | (_) | | | | *
# *  \___\___/|_| |_| |_|_| |_| |_|\___/|_| |_| *
# *                                             *
# ===- system_tests/broker/common.py --------------------------------------===//
#
#  Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
#  See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
#  information.
#  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
