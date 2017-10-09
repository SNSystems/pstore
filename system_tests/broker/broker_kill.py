#!/usr/bin/env python
#  coding=utf-8

from __future__ import print_function

import argparse
import collections
import os.path
import platform
import signal
import subprocess
import sys

is_windows = platform.system () == 'Windows'


# TODO: shared with broker1.py
def executable (path):
    if is_windows:
        path += '.exe'
    return path


# TODO: shared with broker1.py
def pipe_root_dir ():
    return r"\\.\pipe" if is_windows else '/tmp'


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

    broker_command = [paths.broker, '--pipe-path', os.path.join (pipe_root_dir (), 'pstore_broker_kill')]
    broker_process = subprocess.Popen (args=broker_command, stdout=subprocess.PIPE, universal_newlines=True, bufsize=1,
                                       creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if is_windows else 0)
    poker_process = subprocess.Popen (args=[paths.poker, "ECHO", "ready"])
    poker_process.wait ()
    print ("done initial wait: broker is up")

    broker_process.send_signal (signal.CTRL_BREAK_EVENT if is_windows else signal.SIGTERM)
    print ("Sent SIGTERM. Waiting for broker to exit.")

    broker_process.wait ()
    print ("Broker exited. Done.")

    return exit_code


if __name__ == '__main__':
    sys.exit (main (sys.argv [1:]))

# eof broker_kill.py
