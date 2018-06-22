#!/usr/bin/env python
#*  _               _               _    _ _ _  *
#* | |__  _ __ ___ | | _____ _ __  | | _(_) | | *
#* | '_ \| '__/ _ \| |/ / _ \ '__| | |/ / | | | *
#* | |_) | | | (_) |   <  __/ |    |   <| | | | *
#* |_.__/|_|  \___/|_|\_\___|_|    |_|\_\_|_|_| *
#*                                              *
#===- system_tests/broker/broker_kill.py ----------------------------------===//
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

from __future__ import print_function

import argparse
import collections
import os.path
import platform
import signal
import subprocess
import sys

PLATFORM_NAME = platform.system ()
IS_WINDOWS = PLATFORM_NAME == 'Windows'
IS_CYGWIN = PLATFORM_NAME.startswith ('CYGWIN_NT-')


# TODO: shared with broker1.py
def executable (path):
    if IS_WINDOWS or IS_CYGWIN:
        path += '.exe'
    return path


# TODO: shared with broker1.py
def pipe_root_dir ():
    return r'\\.\pipe' if IS_WINDOWS or IS_CYGWIN else '/tmp'


ToolPaths = collections.namedtuple ('ToolPaths', ['broker', 'poker'])


def get_tool_paths (exe_path):
    broker_path = os.path.join (exe_path, executable ('pstore-brokerd'))
    if not os.path.exists (broker_path):
        raise RuntimeError ('Did not find broker executable at "%s"' % broker_path)

    poker_path = os.path.join (exe_path, executable ('pstore-broker-poker'))
    if not os.path.exists (poker_path):
        print ('Did not find broker-poker executable at "%s"' % poker_path, file=sys.stderr)
        return 1

    return ToolPaths (broker=broker_path, poker=poker_path)


def main (argv):
    exit_code = 0
    argv0 = sys.argv [0]
    parser = argparse.ArgumentParser (description='Test the broker by using the poker to fire messages at it.')
    parser.add_argument ('exe_path', help='The path of the pstore binaries')
    # parser.add_argument ('--timeout', help='Process timeout in seconds', type=float, default=DEFAULT_PROCESS_TIMEOUT)
    args = parser.parse_args (args=argv)

    paths = get_tool_paths (args.exe_path)

    pipe_path = os.path.join (pipe_root_dir (), 'pstore_broker_kill')
    broker_command = [paths.broker, '--pipe-path', pipe_path]
    print ("Popen: ", ' '.join (broker_command))
    broker_process = subprocess.Popen (args=broker_command, stdout=subprocess.PIPE, universal_newlines=True, bufsize=1,
                                       creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if IS_WINDOWS else 0)

    poker_command = [paths.poker, '--pipe-path', pipe_path, "ECHO", "ready"]
    print ("Popen: ", ' '.join (poker_command))
    poker_process = subprocess.Popen (args=poker_command)

    poker_process.wait ()
    print ("done initial wait: broker is up")

    broker_process.send_signal (signal.CTRL_BREAK_EVENT if IS_WINDOWS else signal.SIGTERM)
    print ("Sent SIGTERM. Waiting for broker to exit.")

    broker_process.wait ()
    print ("Broker exited. Done.")

    return exit_code


if __name__ == '__main__':
    sys.exit (main (sys.argv [1:]))

# eof: system_tests/broker/broker_kill.py
