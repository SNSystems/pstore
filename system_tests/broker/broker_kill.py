#!/usr/bin/env python
# *  _               _               _    _ _ _  *
# * | |__  _ __ ___ | | _____ _ __  | | _(_) | | *
# * | '_ \| '__/ _ \| |/ / _ \ '__| | |/ / | | | *
# * | |_) | | | (_) |   <  __/ |    |   <| | | | *
# * |_.__/|_|  \___/|_|\_\___|_|    |_|\_\_|_|_| *
# *                                              *
# ===- system_tests/broker/broker_kill.py ---------------------------------===//
#  Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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

import argparse
import collections
import os.path
import signal
import subprocess
import sys
import time

# Local imports
import common
import timed_process

ToolPaths = collections.namedtuple('ToolPaths', ['broker', 'poker'])


def get_tool_paths(exe_path):
    broker_path = os.path.join(exe_path, common.executable('pstore-brokerd'))
    if not os.path.exists(broker_path):
        raise RuntimeError('Did not find broker executable at "%s"' % broker_path)

    poker_path = os.path.join(exe_path, common.executable('pstore-broker-poker'))
    if not os.path.exists(poker_path):
        raise RuntimeError('Did not find broker-poker executable at "%s"' % poker_path)

    return ToolPaths(broker=broker_path, poker=poker_path)


def main(argv):
    exit_code = 0
    argv0 = sys.argv[0]
    parser = argparse.ArgumentParser(description='Test the broker by using the poker to fire messages at it.')
    parser.add_argument('exe_path', help='The path of the pstore binaries')
    parser.add_argument('--timeout', help='Process timeout in seconds', type=float,
                        default=timed_process.DEFAULT_PROCESS_TIMEOUT)
    args = parser.parse_args(args=argv)

    paths = get_tool_paths(args.exe_path)

    pipe_path = os.path.join(common.pipe_root_dir(), 'pstore_broker_kill')
    broker_command = [paths.broker, '--pipe-path', pipe_path, '--disable-http']
    print("Popen: ", ' '.join(broker_command), file=sys.stderr)

    broker_process = timed_process.TimedProcess(args=broker_command,
                                                timeout=args.timeout,
                                                name='broker',
                                                creation_flags=subprocess.CREATE_NEW_PROCESS_GROUP if common.IS_WINDOWS else 0)
    broker_process.start()

    # TODO: this is a crude way to know whether the broker is up: we don't know how long the system
    # will take to start the process before it actually gets to do any work.
    time.sleep(2)  # Wait until the broker is alive.

    broker_process.send_signal(signal.CTRL_BREAK_EVENT if common.IS_WINDOWS else signal.SIGTERM)
    print("Sent SIGTERM. Waiting for broker to exit.", file=sys.stderr)

    broker_process.join()
    print("Broker exited. Done.", file=sys.stderr)

    common.report_error(argv0, 'broker', broker_process)
    print(broker_process.output())

    return exit_code


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
