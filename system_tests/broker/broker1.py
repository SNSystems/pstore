#!/usr/bin/env python
# *  _               _             _  *
# * | |__  _ __ ___ | | _____ _ __/ | *
# * | '_ \| '__/ _ \| |/ / _ \ '__| | *
# * | |_) | | | (_) |   <  __/ |  | | *
# * |_.__/|_|  \___/|_|\_\___|_|  |_| *
# *                                   *
# ===- system_tests/broker/broker1.py -------------------------------------===//
#  Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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

"""A test which runs pstore broker and its "broker poker" utility to fire
messages at it. The outer test code verifies that the correct output was
produced.
"""

from __future__ import print_function
import argparse
import os.path
import sys

# Local imports
import common
import timed_process

PIPE_PATH = os.path.join(common.pipe_root_dir(), 'broker1.pipe')

# Arguments which will cause the broker-poker to tell the broker to echo 100
# sequentially numbered messages to stdout before exiting.
POKER_ARGS = ['--flood', '100', '--kill', '--pipe-path', PIPE_PATH]

BROKER_ARGS = ['--pipe-path', PIPE_PATH]


def main(argv):
    exit_code = 0
    argv0 = sys.argv[0]

    parser = argparse.ArgumentParser(description='Test the broker by using the poker to fire messages at it.')
    parser.add_argument('exe_path', help='The path of the pstore binaries')
    parser.add_argument('--timeout', help='Process timeout in seconds', type=float,
                        default=timed_process.DEFAULT_PROCESS_TIMEOUT)
    args = parser.parse_args(args=argv)

    # Before starting the broker and the broker-poker I need to check that both binaries are available.
    # This is to minimize the risk that the broker is started, but the poker cannot be found. If this
    # happens, the broker will never receive the command telling it to quit, and the test will hang.
    broker_path = os.path.join(args.exe_path, common.executable('pstore-brokerd'))
    poker_path = os.path.join(args.exe_path, common.executable('pstore-broker-poker'))
    if not os.path.exists(broker_path):
        print('did not find broker executable at "%s"' % broker_path, file=sys.stderr)
        return 1
    if not os.path.exists(poker_path):
        print('did not find broker-poker executable at "%s"' % poker_path, file=sys.stderr)
        return 1

    # Start both processes and wait for them to exit.
    cmd = [broker_path] + BROKER_ARGS
    print("Running broker: %s" % (cmd,), file=sys.stderr)
    broker = timed_process.TimedProcess(args=cmd, timeout=args.timeout, name='broker')
    broker.start()

    cmd = [poker_path] + POKER_ARGS
    print("Running poker: %s" % (cmd,), file=sys.stderr)
    poker = timed_process.TimedProcess(args=cmd, timeout=args.timeout, name='poker')
    poker.start()

    print("%s: waiting for poker to exit" % (argv0,), file=sys.stderr)
    poker.join(args.timeout)
    # If the poker failed for some reason then kill the broker early.
    if poker.did_timeout() or poker.exception():
        print("%s: poker failed so killing broker" % (argv0,), file=sys.stderr)
        broker.timeout()

    print("%s: waiting for broker to exit" % (argv0,), file=sys.stderr)
    broker.join(args.timeout)

    print("%s: done" % (argv0,), file=sys.stderr)

    if not common.report_error(argv0, "broker", broker):
        exit_code = 1
    else:
        print("broker exited successfully")

    if not common.report_error(argv0, "poker", poker):
        exit_code = 1
    else:
        print("poker exited successfully")

    # Note that the output is sorted to guarantee stability: the order in
    # which the broker processes incoming messages may not match the order in
    # which they were sent.
    print('\n'.join(sorted(broker.output().splitlines())))
    return exit_code


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
