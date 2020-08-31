#!/usr/bin/env python
# *  _     _   _             _  *
# * | |__ | |_| |_ _ __   __| | *
# * | '_ \| __| __| '_ \ / _` | *
# * | | | | |_| |_| |_) | (_| | *
# * |_| |_|\__|\__| .__/ \__,_| *
# *               |_|           *
# ===- system_tests/broker/httpd.py ---------------------------------------===//
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
from __future__ import print_function

import argparse
import functools
import os.path
import signal
import subprocess
import sys
import time

if sys.version_info.major >= 3:
    import http.client as httplib
else:
    import httplib

# Local imports
import common
import timed_process

EXIT_SUCCESS = 0
EXIT_FAILURE = 1

FIRST_EPHEMERAL_PORT = 49152
MAX_PORT = 65535


def get_broker_path(exe_path):
    """
    Returns the path to the broker executable.
    :param exe_path: The path in which the binaries were built.
    :return: The path to the broker executable.
    """

    path = os.path.join(exe_path, common.executable('pstore-brokerd'))
    if not os.path.exists(path):
        raise RuntimeError('Did not find broker executable at "%s"' % path)
    return path


def default_port():
    """
    Returns a default HTTP port number to be assigned to the broker. This value is intended to be
    stable but likely different to port numbers used by other tests to avoid collisions if the tests
    are running simultaneously.

    :return: A default port number.
    """

    return hash(os.path.basename(__file__)) % (MAX_PORT - FIRST_EPHEMERAL_PORT) + FIRST_EPHEMERAL_PORT


def creation_flags():
    return subprocess.CREATE_NEW_PROCESS_GROUP if common.IS_WINDOWS else 0


def start_broker(broker_path, port, timeout=60, switches=None):
    """
    Starts the broker process.

    :param broker_path: The path of the broker executable.
    :param port: The port number on which the HTTP server should listen.
    :param timeout: If the process is silent for longer than the timeout value (in seconds), the
        process is killed and the test failed.
    :param switches: A dictionary containing additional switches to pass to the broker when it is
        started.
    :return: An instance of timed_process.TimedProcess.
    """

    if switches is None:
        switches = {}

    # We must set pipe-path to avoid this broker interacting with another.
    switches.setdefault('--pipe-path', os.path.join(common.pipe_root_dir(),
                                                    os.path.basename(__file__)))
    # We must set the port on which the HTTP server will listen for the same reason.
    switches.setdefault('--http-port', str(port))
    # Turn the switches dictinary into a list.
    arguments = list(functools.reduce(lambda k, v: k + v, switches.items()))
    print('broker args: %s' % arguments, file=sys.stderr)
    # Start the broker.
    process = timed_process.TimedProcess(args=[broker_path] + arguments,
                                         timeout=timeout,
                                         name='broker',
                                         creation_flags=creation_flags())
    process.start()
    # TODO: this is a crude way to wait for the broker to be fully up: we don't know how long the
    # system will take to start the process before it actually gets to do any work.
    time.sleep(2)  # Wait until the broker is alive.
    print("Broker alive.", file=sys.stderr)
    return process


def kill_broker(process):
    """
    Kills a running process.

    :param process: An instance of timed_process.TimedProcess.
    :return: Nothing
    """

    process.send_signal(signal.CTRL_BREAK_EVENT if common.IS_WINDOWS else signal.SIGTERM)
    print("Sent SIGTERM. Waiting for broker to exit.", file=sys.stderr)
    process.join()
    print("Broker exited. Done.", file=sys.stderr)


def http_get(host, port, method="GET", path="/index.html"):
    print("Connecting to broker at %s:%d" % (host, port), file=sys.stderr)
    conn = httplib.HTTPConnection(host, port)
    conn.connect()

    print("Request %s %s from %s:%d" % (method, path, host, port), file=sys.stderr)
    conn.request(method, path)
    reply = conn.getresponse().read()

    conn.close()
    return reply



def check_reply (reply):
    return "Hello from the pstore HTTP server" in reply


def main(broker_path, host, port, timeout):
    exit_code = EXIT_SUCCESS
    print('Using HTTP port %d' % port, file=sys.stderr)
    broker_process = start_broker(broker_path, port, timeout)
    try:
        reply = http_get(host, port)
        print("Reply was: {0}".format (reply), file=sys.stderr)
        if not check_reply (reply):
            exit_code = EXIT_FAILURE
    finally:
        kill_broker(broker_process)
    return exit_code


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Check basic behavior of the pstore HTTP server')
    parser.add_argument('exe_path', help='The path of the pstore binaries')
    parser.add_argument('--port', help='The broker\' HTTP port', default=default_port())
    parser.add_argument('--timeout', help='Process timeout in seconds', type=float,
                        default=timed_process.DEFAULT_PROCESS_TIMEOUT)
    args = parser.parse_args()
    sys.exit(main(get_broker_path(args.exe_path),
                  host="localhost",
                  port=args.port,
                  timeout=args.timeout))
