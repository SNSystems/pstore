#!/usr/bin/env python
# *  _     _   _             _  *
# * | |__ | |_| |_ _ __   __| | *
# * | '_ \| __| __| '_ \ / _` | *
# * | | | | |_| |_| |_) | (_| | *
# * |_| |_|\__|\__| .__/ \__,_| *
# *               |_|           *
# ===- system_tests/broker/httpd.py ---------------------------------------===//
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
import argparse
import collections
import functools
import logging
import os.path
import re
import signal
import subprocess
import sys
import threading

if sys.version_info.major >= 3:
    import http.client as httplib
else:
    import httplib

# Local imports
import common
from watchdog_timer import get_process_output, WatchdogTimer

EXIT_SUCCESS = 0
EXIT_FAILURE = 1

# The default process timeout in seconds
DEFAULT_PROCESS_TIMEOUT = 3 * 60.0

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


def creation_flags():
    return subprocess.CREATE_NEW_PROCESS_GROUP if common.IS_WINDOWS else 0


BrokerInfo = collections.namedtuple ("BrokerInfo", ['watchdog', 'process', 'port'])

def kill(process):
    logging.error ('Watchdog timeout: killing process')
    process.kill ()


def start_broker(broker_path, timeout=60, switches=None):
    """
    Starts the broker process.

    :param broker_path: The path of the broker executable.
    :param timeout: If the process is silent for longer than the timeout value (in seconds), the
        process is killed and the test failed.
    :param switches: A dictionary containing additional switches to pass to the broker when it is
        started.
    :return: An instance of BrokerInfo containing the watchdog, process, and the port number on
        which the HTTP server is listening.
    """

    if switches is None:
        switches = {}

    # We must set pipe-path to avoid this broker interacting with another.
    switches.setdefault('--pipe-path', os.path.join(common.pipe_root_dir(),
                                                    os.path.basename(__file__)))

    # Tell the HTTP server to use port 0. This will cause it to select an available ephemeral port
    # number.
    switches.setdefault('--http-port', str(0))

    # Turn the switches dictionary into a list.
    arguments = list(functools.reduce(lambda k, v: k + v, switches.items()))
    # We want to know when the HTTP server is up and which port it is using.
    arguments.append('--announce-http-port')
    logging.info ('broker args: %s', arguments)

    # Start the broker.
    process = subprocess.Popen(args = [broker_path] + arguments,
                               stdin = subprocess.PIPE,
                               stdout = subprocess.PIPE,
                               stderr = subprocess.STDOUT,
                               creationflags = creation_flags(),
                               universal_newlines = True)

    watchdog = WatchdogTimer(timeout=timeout, callback=kill, args=(process,), name='broker-watchdog')

    while True:
        line = get_process_output(process, watchdog)
        m = re.match ('^HTTP listening on port ([0-9]+)$', line)
        if m is not None:
            port = int (m.group (1))
            return BrokerInfo(watchdog, process, port)


def kill_broker(process):
    """
    Kills a running process.

    :param process: An instance of subprocess.Popen
    :return: Nothing
    """

    process.send_signal(signal.CTRL_BREAK_EVENT if common.IS_WINDOWS else signal.SIGTERM)
    logging.info ('Sent SIGTERM. Waiting for broker to exit.')
    timeout_secs = 15
    timer = threading.Timer(timeout_secs, kill)
    try:
        timer.start()
        stdout, stderr = process.communicate()
    finally:
        timer.cancel()

    logging.info('Broker exited. Done.')


def http_get(host, port, method="GET", path="/index.html"):
    logging.info('Connecting to broker at %s:%d', host, port)
    conn = httplib.HTTPConnection(host, port)
    conn.connect()

    logging.info('Request %s %s from %s:%d', method, path, host, port)
    conn.request(method, path)
    reply = conn.getresponse().read()

    conn.close()
    return reply



def check_reply (reply):
    return "Hello from the pstore HTTP server" in reply


def main(broker_path, host, timeout):
    exit_code = EXIT_SUCCESS
    broker_info = start_broker(broker_path, timeout)
    try:
        reply = http_get(host, broker_info.port).decode("utf-8")
        logging.info ('Reply was: %s', reply)
        if not check_reply(reply):
            exit_code = EXIT_FAILURE
    finally:
        broker_info.watchdog.restart()
        kill_broker(broker_info.process)
    return exit_code


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG,
                        format='%(levelname)s - %(asctime)s - %(threadName)s - %(filename)s:%(lineno)d - %(message)s')
    logging.debug('Starting')

    parser = argparse.ArgumentParser(description='Check basic behavior of the pstore HTTP server')
    parser.add_argument('exe_path', help='The path of the pstore binaries')
    parser.add_argument('--timeout', help='Process timeout in seconds', type=float,
                        default=DEFAULT_PROCESS_TIMEOUT)
    args = parser.parse_args()
    sys.exit(main(get_broker_path(args.exe_path),
                  host="localhost",
                  timeout=args.timeout))
